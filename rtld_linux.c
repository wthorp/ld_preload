#define _GNU_SOURCE

#include "rtld.h"

#include <dlfcn.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/statvfs.h>

typedef int (*xstat_fn)(int, const char *, struct stat *);
typedef int (*access_fn)(const char *, int);
typedef int (*fxstat_fn)(int, int, struct stat *);
typedef int (*fxstatat_fn)(int, int, const char *, struct stat *, int);
typedef int (*newfstatat_fn)(int, const char *, struct stat *, int);
typedef int (*close_fn)(int);
typedef int (*creat_fn)(const char *, mode_t);
typedef int (*fstat_fn)(int, struct stat *);
typedef int (*chdir_fn)(const char *);
typedef int (*execve_fn)(const char *, char *const[], char *const[]);
typedef int (*link_fn)(const char *, const char *);
typedef int (*linkat_fn)(int, const char *, int, const char *, int);
typedef off_t (*lseek_fn)(int, off_t, int);
typedef int (*mkdir_fn)(const char *, mode_t);
typedef int (*mkdirat_fn)(int, const char *, mode_t);
typedef int (*open_fn)(const char *, int, ...);
typedef int (*openat_fn)(int, const char *, int, ...);
typedef int (*openat2_fn)(int, const char *, const struct open_how *, size_t);
typedef int (*faccessat_fn)(int, const char *, int, int);
typedef int (*fchmodat_fn)(int, const char *, mode_t, int);
typedef ssize_t (*pread_fn)(int, void *, size_t, off_t);
typedef ssize_t (*pwrite_fn)(int, const void *, size_t, off_t);
typedef ssize_t (*read_fn)(int, void *, size_t);
typedef ssize_t (*readlink_fn)(const char *, char *, size_t);
typedef ssize_t (*readlinkat_fn)(int, const char *, char *, size_t);
typedef int (*rename_fn)(const char *, const char *);
typedef int (*renameat_fn)(int, const char *, int, const char *);
typedef int (*renameat2_fn)(int, const char *, int, const char *, unsigned int);
typedef int (*rmdir_fn)(const char *);
typedef char *(*getcwd_fn)(char *, size_t);
typedef int (*stat_fn)(const char *, struct stat *);
typedef int (*statx_fn)(int, const char *, int, unsigned int, struct statx *);
typedef int (*symlink_fn)(const char *, const char *);
typedef int (*symlinkat_fn)(const char *, int, const char *);
typedef int (*truncate_fn)(const char *, off_t);
typedef int (*unlink_fn)(const char *);
typedef int (*unlinkat_fn)(int, const char *, int);
typedef int (*utimensat_fn)(int, const char *, const struct timespec[2], int);
typedef int (*statfs_fn)(const char *, struct statvfs *);
typedef ssize_t (*write_fn)(int, const void *, size_t);
typedef int (*euidaccess_fn)(const char *, int);

static pthread_once_t resolve_once = PTHREAD_ONCE_INIT;
static pthread_once_t rewrite_once = PTHREAD_ONCE_INIT;

static fxstat_fn real___fxstat;
static fxstatat_fn real___fxstatat;
static newfstatat_fn real_newfstatat;
static xstat_fn real___lxstat;
static xstat_fn real___xstat;
static access_fn real_access;
static chdir_fn real_chdir;
static execve_fn real_execve;
static close_fn real_close;
static creat_fn real_creat;
static fstat_fn real_fstat;
static faccessat_fn real_faccessat;
static fchmodat_fn real_fchmodat;
static link_fn real_link;
static linkat_fn real_linkat;
static lseek_fn real_lseek;
static mkdir_fn real_mkdir;
static mkdirat_fn real_mkdirat;
static open_fn real_open;
static openat_fn real_openat;
static openat2_fn real_openat2;
static pread_fn real_pread;
static pwrite_fn real_pwrite;
static getcwd_fn real_getcwd;
static read_fn real_read;
static readlink_fn real_readlink;
static readlinkat_fn real_readlinkat;
static rename_fn real_rename;
static renameat_fn real_renameat;
static renameat2_fn real_renameat2;
static rmdir_fn real_rmdir;
static stat_fn real_lstat;
static stat_fn real_stat;
static statx_fn real_statx;
static statfs_fn real_statfs;
static symlink_fn real_symlink;
static symlinkat_fn real_symlinkat;
static truncate_fn real_truncate;
static unlink_fn real_unlink;
static unlinkat_fn real_unlinkat;
static utimensat_fn real_utimensat;
static euidaccess_fn real_euidaccess;
static write_fn real_write;

struct rewrite_policy {
    int enabled;
    size_t from_len;
    size_t to_len;
    char from[PATH_MAX];
    char to[PATH_MAX];
};

static struct rewrite_policy rewrite_policy;

static void resolve_symbols(void) {
    real___fxstat = (fxstat_fn)dlsym(RTLD_NEXT, "__fxstat");
    real___fxstatat = (fxstatat_fn)dlsym(RTLD_NEXT, "__fxstatat");
    real_newfstatat = (newfstatat_fn)dlsym(RTLD_NEXT, "newfstatat");
    real___lxstat = (xstat_fn)dlsym(RTLD_NEXT, "__lxstat");
    real___xstat = (xstat_fn)dlsym(RTLD_NEXT, "__xstat");
    real_access = (access_fn)dlsym(RTLD_NEXT, "access");
    real_chdir = (chdir_fn)dlsym(RTLD_NEXT, "chdir");
    real_execve = (execve_fn)dlsym(RTLD_NEXT, "execve");
    real_close = (close_fn)dlsym(RTLD_NEXT, "close");
    real_creat = (creat_fn)dlsym(RTLD_NEXT, "creat");
    real_fstat = (fstat_fn)dlsym(RTLD_NEXT, "fstat");
    real_faccessat = (faccessat_fn)dlsym(RTLD_NEXT, "faccessat");
    real_fchmodat = (fchmodat_fn)dlsym(RTLD_NEXT, "fchmodat");
    real_link = (link_fn)dlsym(RTLD_NEXT, "link");
    real_linkat = (linkat_fn)dlsym(RTLD_NEXT, "linkat");
    real_lseek = (lseek_fn)dlsym(RTLD_NEXT, "lseek");
    real_mkdir = (mkdir_fn)dlsym(RTLD_NEXT, "mkdir");
    real_mkdirat = (mkdirat_fn)dlsym(RTLD_NEXT, "mkdirat");
    real_open = (open_fn)dlsym(RTLD_NEXT, "open");
    real_openat = (openat_fn)dlsym(RTLD_NEXT, "openat");
    real_openat2 = (openat2_fn)dlsym(RTLD_NEXT, "openat2");
    real_pread = (pread_fn)dlsym(RTLD_NEXT, "pread");
    real_pwrite = (pwrite_fn)dlsym(RTLD_NEXT, "pwrite");
    real_getcwd = (getcwd_fn)dlsym(RTLD_NEXT, "getcwd");
    real_read = (read_fn)dlsym(RTLD_NEXT, "read");
    real_readlink = (readlink_fn)dlsym(RTLD_NEXT, "readlink");
    real_readlinkat = (readlinkat_fn)dlsym(RTLD_NEXT, "readlinkat");
    real_rename = (rename_fn)dlsym(RTLD_NEXT, "rename");
    real_renameat = (renameat_fn)dlsym(RTLD_NEXT, "renameat");
    real_renameat2 = (renameat2_fn)dlsym(RTLD_NEXT, "renameat2");
    real_rmdir = (rmdir_fn)dlsym(RTLD_NEXT, "rmdir");
    real_lstat = (stat_fn)dlsym(RTLD_NEXT, "lstat");
    real_stat = (stat_fn)dlsym(RTLD_NEXT, "stat");
    real_statx = (statx_fn)dlsym(RTLD_NEXT, "statx");
    real_statfs = (statfs_fn)dlsym(RTLD_NEXT, "statfs");
    real_symlink = (symlink_fn)dlsym(RTLD_NEXT, "symlink");
    real_symlinkat = (symlinkat_fn)dlsym(RTLD_NEXT, "symlinkat");
    real_truncate = (truncate_fn)dlsym(RTLD_NEXT, "truncate");
    real_unlink = (unlink_fn)dlsym(RTLD_NEXT, "unlink");
    real_unlinkat = (unlinkat_fn)dlsym(RTLD_NEXT, "unlinkat");
    real_utimensat = (utimensat_fn)dlsym(RTLD_NEXT, "utimensat");
    real_euidaccess = (euidaccess_fn)dlsym(RTLD_NEXT, "euidaccess");
    real_write = (write_fn)dlsym(RTLD_NEXT, "write");
}

static void ensure_symbols(void) { pthread_once(&resolve_once, resolve_symbols); }

static int clean_path_into(const char *src, char *dst, size_t dst_len) {
    size_t seg_starts[PATH_MAX];
    size_t seg_count = 0;
    size_t out = 0;
    int absolute;

    if (src == NULL || dst == NULL || dst_len == 0) {
        return -1;
    }

    absolute = src[0] == '/';
    if (absolute) {
        if (dst_len < 2) {
            return -1;
        }
        dst[out++] = '/';
    }

    while (*src != '\0') {
        const char *start;
        size_t seg_len;

        while (*src == '/') {
            src++;
        }
        if (*src == '\0') {
            break;
        }

        start = src;
        while (*src != '\0' && *src != '/') {
            src++;
        }
        seg_len = (size_t)(src - start);

        if (seg_len == 1 && start[0] == '.') {
            continue;
        }

        if (seg_len == 2 && start[0] == '.' && start[1] == '.') {
            if (seg_count > 0) {
                out = seg_starts[--seg_count];
                if (out == 0 && absolute) {
                    dst[out++] = '/';
                }
                continue;
            }
            if (absolute) {
                continue;
            }
        }

        if (seg_count >= PATH_MAX) {
            return -1;
        }
        if (out != 0 && dst[out - 1] != '/') {
            if (out + 1 >= dst_len) {
                return -1;
            }
            dst[out++] = '/';
        }
        seg_starts[seg_count++] = out;
        if (out + seg_len >= dst_len) {
            return -1;
        }
        memcpy(dst + out, start, seg_len);
        out += seg_len;
    }

    if (out == 0) {
        if (dst_len < 2) {
            return -1;
        }
        if (absolute) {
            dst[out++] = '/';
        } else {
            dst[out++] = '.';
        }
    }

    dst[out] = '\0';
    return 0;
}

static int join_and_clean_path(const char *base, const char *path, char *dst, size_t dst_len) {
    char joined[PATH_MAX];

    if (base == NULL || path == NULL || dst == NULL) {
        return -1;
    }
    if (snprintf(joined, sizeof(joined), "%s/%s", base, path) >= (int)sizeof(joined)) {
        return -1;
    }
    return clean_path_into(joined, dst, dst_len);
}

static int path_matches_prefix(const char *path, const char *prefix, size_t prefix_len) {
    if (strncmp(path, prefix, prefix_len) != 0) {
        return 0;
    }
    return path[prefix_len] == '\0' || path[prefix_len] == '/';
}

static int substitute_prefix(const char *path, char *dst, size_t dst_len) {
    size_t suffix_len;

    if (!rewrite_policy.enabled ||
        !path_matches_prefix(path, rewrite_policy.from, rewrite_policy.from_len)) {
        return -1;
    }

    suffix_len = strlen(path + rewrite_policy.from_len);
    if (rewrite_policy.to_len + suffix_len + 1 > dst_len) {
        return -1;
    }
    memcpy(dst, rewrite_policy.to, rewrite_policy.to_len);
    memcpy(dst + rewrite_policy.to_len, path + rewrite_policy.from_len, suffix_len + 1);
    return 0;
}

static int substitute_prefix_reverse(const char *path, char *dst, size_t dst_len) {
    size_t suffix_len;

    if (!rewrite_policy.enabled || !path_matches_prefix(path, rewrite_policy.to, rewrite_policy.to_len)) {
        return -1;
    }

    suffix_len = strlen(path + rewrite_policy.to_len);
    if (rewrite_policy.from_len + suffix_len + 1 > dst_len) {
        return -1;
    }
    memcpy(dst, rewrite_policy.from, rewrite_policy.from_len);
    memcpy(dst + rewrite_policy.from_len, path + rewrite_policy.to_len, suffix_len + 1);
    return 0;
}

static void init_rewrite_policy(void) {
    const char *from_env;
    const char *to_env;
    char cleaned_from[PATH_MAX];
    char cleaned_to[PATH_MAX];

    rewrite_policy.enabled = 0;
    rewrite_policy.from[0] = '\0';
    rewrite_policy.to[0] = '\0';
    rewrite_policy.from_len = 0;
    rewrite_policy.to_len = 0;

    from_env = getenv("LD_PRELOAD_REWRITE_FROM");
    to_env = getenv("LD_PRELOAD_REWRITE_TO");
    if (from_env == NULL || to_env == NULL || from_env[0] == '\0' || to_env[0] == '\0') {
        return;
    }
    if (from_env[0] != '/' || to_env[0] != '/') {
        return;
    }
    if (clean_path_into(from_env, cleaned_from, sizeof(cleaned_from)) != 0 ||
        clean_path_into(to_env, cleaned_to, sizeof(cleaned_to)) != 0) {
        return;
    }
    if (cleaned_from[0] != '/' || cleaned_to[0] != '/') {
        return;
    }

    memcpy(rewrite_policy.from, cleaned_from, sizeof(cleaned_from));
    memcpy(rewrite_policy.to, cleaned_to, sizeof(cleaned_to));
    rewrite_policy.from_len = strlen(rewrite_policy.from);
    rewrite_policy.to_len = strlen(rewrite_policy.to);
    rewrite_policy.enabled = 1;
}

static const struct rewrite_policy *get_rewrite_policy(void) {
    pthread_once(&rewrite_once, init_rewrite_policy);
    return &rewrite_policy;
}

static int rewrite_absolute_path(const char *pathname, char *dst, size_t dst_len) {
    char cleaned[PATH_MAX];

    if (pathname == NULL || pathname[0] != '/') {
        return 0;
    }
    if (!get_rewrite_policy()->enabled) {
        return 0;
    }
    if (clean_path_into(pathname, cleaned, sizeof(cleaned)) != 0) {
        return 0;
    }
    if (substitute_prefix(cleaned, dst, dst_len) != 0) {
        return 0;
    }
    return 1;
}

static int rewrite_relative_path_from_dirfd(int dirfd, const char *pathname, char *dst, size_t dst_len) {
    char base[PATH_MAX];
    char resolved[PATH_MAX];

    if (pathname == NULL || pathname[0] == '/') {
        return 0;
    }
    if (!get_rewrite_policy()->enabled) {
        return 0;
    }

    ensure_symbols();
    if (dirfd == AT_FDCWD) {
        if (real_getcwd == NULL) {
            return 0;
        }
        if (real_getcwd(base, sizeof(base)) == NULL) {
            return 0;
        }
    } else {
        char fd_path[64];
        ssize_t n;

        if (snprintf(fd_path, sizeof(fd_path), "/proc/self/fd/%d", dirfd) >= (int)sizeof(fd_path)) {
            return 0;
        }
        ensure_symbols();
        if (real_readlink == NULL) {
            return 0;
        }
        n = real_readlink(fd_path, base, sizeof(base) - 1);
        if (n < 0 || n >= (ssize_t)sizeof(base) - 1) {
            return 0;
        }
        base[n] = '\0';
        if (base[0] != '/') {
            return 0;
        }
        if (clean_path_into(base, resolved, sizeof(resolved)) != 0) {
            return 0;
        }
        memcpy(base, resolved, sizeof(base));
    }

    if (join_and_clean_path(base, pathname, resolved, sizeof(resolved)) != 0) {
        return 0;
    }
    if (substitute_prefix(resolved, dst, dst_len) != 0) {
        return 0;
    }
    return 1;
}

static const char *rewrite_path_argument(const char *pathname, char *buf, size_t buf_len) {
    if (pathname == NULL) {
        return pathname;
    }
    if (rewrite_absolute_path(pathname, buf, buf_len)) {
        return buf;
    }
    return pathname;
}

static const char *rewrite_cwd_relative_argument(const char *pathname, char *buf, size_t buf_len) {
    if (pathname == NULL) {
        return pathname;
    }
    if (pathname[0] == '/') {
        return rewrite_path_argument(pathname, buf, buf_len);
    }
    if (rewrite_relative_path_from_dirfd(AT_FDCWD, pathname, buf, buf_len)) {
        return buf;
    }
    return pathname;
}

static const char *rewrite_at_argument(int *dirfd, const char *pathname, char *buf, size_t buf_len) {
    if (pathname == NULL) {
        return pathname;
    }
    if (pathname[0] == '/') {
        return rewrite_path_argument(pathname, buf, buf_len);
    }
    if (rewrite_relative_path_from_dirfd(*dirfd, pathname, buf, buf_len)) {
        *dirfd = AT_FDCWD;
        return buf;
    }
    return pathname;
}

#define REQUIRE_SYMBOL(sym)                                                                        \
    do {                                                                                           \
        ensure_symbols();                                                                          \
        if ((sym) == NULL) {                                                                       \
            errno = ENOSYS;                                                                        \
            return -1;                                                                             \
        }                                                                                          \
    } while (0)

#define REQUIRE_SYMBOL_PTR(sym)                                                                     \
    do {                                                                                           \
        ensure_symbols();                                                                          \
        if ((sym) == NULL) {                                                                       \
            errno = ENOSYS;                                                                        \
            return NULL;                                                                           \
        }                                                                                          \
    } while (0)

int orig___fxstat(int ver, int fd, struct stat *cstat) {
    REQUIRE_SYMBOL(real___fxstat);
    return real___fxstat(ver, fd, cstat);
}

int orig___fxstatat(int ver, int dirfd, const char *pathname, struct stat *cstat, int flags) {
    REQUIRE_SYMBOL(real___fxstatat);
    return real___fxstatat(ver, dirfd, pathname, cstat, flags);
}

int orig___lxstat(int ver, const char *pathname, struct stat *cstat) {
    REQUIRE_SYMBOL(real___lxstat);
    return real___lxstat(ver, pathname, cstat);
}

int orig___xstat(int ver, const char *pathname, struct stat *cstat) {
    REQUIRE_SYMBOL(real___xstat);
    return real___xstat(ver, pathname, cstat);
}

int orig_access(const char *pathname, int mode) {
    REQUIRE_SYMBOL(real_access);
    return real_access(pathname, mode);
}

int orig_chdir(const char *path) {
    REQUIRE_SYMBOL(real_chdir);
    return real_chdir(path);
}

int orig_execve(const char *pathname, char *const argv[], char *const envp[]) {
    REQUIRE_SYMBOL(real_execve);
    return real_execve(pathname, argv, envp);
}

int orig_close(int fd) {
    REQUIRE_SYMBOL(real_close);
    return real_close(fd);
}

int orig_creat(const char *pathname, mode_t mode) {
    REQUIRE_SYMBOL(real_creat);
    return real_creat(pathname, mode);
}

int orig_fstat(int fd, struct stat *cstat) {
    REQUIRE_SYMBOL(real_fstat);
    return real_fstat(fd, cstat);
}

int orig_newfstatat(int dirfd, const char *pathname, struct stat *statbuf, int flags) {
    REQUIRE_SYMBOL(real_newfstatat);
    return real_newfstatat(dirfd, pathname, statbuf, flags);
}

int orig_faccessat(int dirfd, const char *pathname, int mode, int flags) {
    REQUIRE_SYMBOL(real_faccessat);
    return real_faccessat(dirfd, pathname, mode, flags);
}

int orig_fchmodat(int dirfd, const char *pathname, mode_t mode, int flags) {
    REQUIRE_SYMBOL(real_fchmodat);
    return real_fchmodat(dirfd, pathname, mode, flags);
}

int orig_link(const char *oldpath, const char *newpath) {
    REQUIRE_SYMBOL(real_link);
    return real_link(oldpath, newpath);
}

int orig_linkat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags) {
    REQUIRE_SYMBOL(real_linkat);
    return real_linkat(olddirfd, oldpath, newdirfd, newpath, flags);
}

off_t orig_lseek(int fd, off_t offset, int whence) {
    REQUIRE_SYMBOL(real_lseek);
    return real_lseek(fd, offset, whence);
}

int orig_mkdir(const char *pathname, mode_t mode) {
    REQUIRE_SYMBOL(real_mkdir);
    return real_mkdir(pathname, mode);
}

int orig_mkdirat(int dirfd, const char *pathname, mode_t mode) {
    REQUIRE_SYMBOL(real_mkdirat);
    return real_mkdirat(dirfd, pathname, mode);
}

int orig_open(const char *pathname, int flags, mode_t mode) {
    REQUIRE_SYMBOL(real_open);
    return real_open(pathname, flags, mode);
}

int orig_openat(int dirfd, const char *pathname, int flags, mode_t mode) {
    REQUIRE_SYMBOL(real_openat);
    return real_openat(dirfd, pathname, flags, mode);
}

int orig_openat2(int dirfd, const char *pathname, const struct open_how *how, size_t size) {
    REQUIRE_SYMBOL(real_openat2);
    return real_openat2(dirfd, pathname, how, size);
}

char *orig_getcwd(char *buf, size_t size) {
    REQUIRE_SYMBOL_PTR(real_getcwd);
    return real_getcwd(buf, size);
}

int orig_euidaccess(const char *pathname, int mode) {
    REQUIRE_SYMBOL(real_euidaccess);
    return real_euidaccess(pathname, mode);
}

ssize_t orig_pread(int fd, void *buf, size_t count, off_t offset) {
    REQUIRE_SYMBOL(real_pread);
    return real_pread(fd, buf, count, offset);
}

ssize_t orig_pwrite(int fd, const void *buf, size_t count, off_t offset) {
    REQUIRE_SYMBOL(real_pwrite);
    return real_pwrite(fd, buf, count, offset);
}

ssize_t orig_read(int fd, void *buf, size_t count) {
    REQUIRE_SYMBOL(real_read);
    return real_read(fd, buf, count);
}

ssize_t orig_readlink(const char *pathname, void *buf, size_t bufsiz) {
    REQUIRE_SYMBOL(real_readlink);
    return real_readlink(pathname, (char *)buf, bufsiz);
}

ssize_t orig_readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz) {
    REQUIRE_SYMBOL(real_readlinkat);
    return real_readlinkat(dirfd, pathname, buf, bufsiz);
}

int orig_rename(const char *oldpath, const char *newpath) {
    REQUIRE_SYMBOL(real_rename);
    return real_rename(oldpath, newpath);
}

int orig_renameat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath) {
    REQUIRE_SYMBOL(real_renameat);
    return real_renameat(olddirfd, oldpath, newdirfd, newpath);
}

int orig_renameat2(int olddirfd, const char *oldpath, int newdirfd, const char *newpath,
                   unsigned int flags) {
    REQUIRE_SYMBOL(real_renameat2);
    return real_renameat2(olddirfd, oldpath, newdirfd, newpath, flags);
}

int orig_rmdir(const char *pathname) {
    REQUIRE_SYMBOL(real_rmdir);
    return real_rmdir(pathname);
}

int orig_lstat(const char *pathname, struct stat *cstat) {
    REQUIRE_SYMBOL(real_lstat);
    return real_lstat(pathname, cstat);
}

int orig_statx(int dirfd, const char *pathname, int flags, unsigned int mask,
               struct statx *statxbuf) {
    REQUIRE_SYMBOL(real_statx);
    return real_statx(dirfd, pathname, flags, mask, statxbuf);
}

int orig_stat(const char *pathname, struct stat *cstat) {
    REQUIRE_SYMBOL(real_stat);
    return real_stat(pathname, cstat);
}

int orig_statfs(const char *path, struct statvfs *buf) {
    REQUIRE_SYMBOL(real_statfs);
    return real_statfs(path, buf);
}

int orig_symlink(const char *target, const char *linkpath) {
    REQUIRE_SYMBOL(real_symlink);
    return real_symlink(target, linkpath);
}

int orig_symlinkat(const char *target, int newdirfd, const char *linkpath) {
    REQUIRE_SYMBOL(real_symlinkat);
    return real_symlinkat(target, newdirfd, linkpath);
}

int orig_truncate(const char *pathname, off_t length) {
    REQUIRE_SYMBOL(real_truncate);
    return real_truncate(pathname, length);
}

int orig_unlink(const char *pathname) {
    REQUIRE_SYMBOL(real_unlink);
    return real_unlink(pathname);
}

int orig_unlinkat(int dirfd, const char *pathname, int flags) {
    REQUIRE_SYMBOL(real_unlinkat);
    return real_unlinkat(dirfd, pathname, flags);
}

int orig_utimensat(int dirfd, const char *pathname, const struct timespec times[2], int flags) {
    REQUIRE_SYMBOL(real_utimensat);
    return real_utimensat(dirfd, pathname, times, flags);
}

ssize_t orig_write(int fd, const void *buf, size_t count) {
    REQUIRE_SYMBOL(real_write);
    return real_write(fd, buf, count);
}

int __fxstat(int ver, int fd, struct stat *cstat) { return orig___fxstat(ver, fd, cstat); }

int __fxstatat(int ver, int dirfd, const char *pathname, struct stat *cstat, int flags) {
    char rewritten[PATH_MAX];
    const char *target = pathname;
    int call_dirfd = dirfd;

    if (pathname != NULL && pathname[0] == '/') {
        target = rewrite_path_argument(pathname, rewritten, sizeof(rewritten));
    } else if (rewrite_relative_path_from_dirfd(dirfd, pathname, rewritten, sizeof(rewritten))) {
        target = rewritten;
        call_dirfd = AT_FDCWD;
    }

    return orig___fxstatat(ver, call_dirfd, target, cstat, flags);
}

int newfstatat(int dirfd, const char *pathname, struct stat *statbuf, int flags) {
    char rewritten[PATH_MAX];
    const char *target = pathname;
    int call_dirfd = dirfd;

    if (pathname != NULL && pathname[0] == '/') {
        target = rewrite_path_argument(pathname, rewritten, sizeof(rewritten));
    } else if (rewrite_relative_path_from_dirfd(dirfd, pathname, rewritten, sizeof(rewritten))) {
        target = rewritten;
        call_dirfd = AT_FDCWD;
    }

    return orig_newfstatat(call_dirfd, target, statbuf, flags);
}

int __lxstat(int ver, const char *pathname, struct stat *cstat) {
    char rewritten[PATH_MAX];
    return orig___lxstat(ver, rewrite_path_argument(pathname, rewritten, sizeof(rewritten)), cstat);
}

int __xstat(int ver, const char *pathname, struct stat *cstat) {
    char rewritten[PATH_MAX];
    return orig___xstat(ver, rewrite_path_argument(pathname, rewritten, sizeof(rewritten)), cstat);
}

int access(const char *pathname, int mode) {
    char rewritten[PATH_MAX];
    return orig_access(rewrite_cwd_relative_argument(pathname, rewritten, sizeof(rewritten)), mode);
}

int chdir(const char *path) {
    char rewritten[PATH_MAX];
    return orig_chdir(rewrite_cwd_relative_argument(path, rewritten, sizeof(rewritten)));
}

int close(int fd) { return orig_close(fd); }

int creat(const char *pathname, mode_t mode) {
    char rewritten[PATH_MAX];
    return orig_creat(rewrite_path_argument(pathname, rewritten, sizeof(rewritten)), mode);
}

int execve(const char *pathname, char *const argv[], char *const envp[]) {
    char rewritten[PATH_MAX];
    return orig_execve(rewrite_cwd_relative_argument(pathname, rewritten, sizeof(rewritten)), argv, envp);
}

int euidaccess(const char *pathname, int mode) {
    char rewritten[PATH_MAX];
    return orig_euidaccess(rewrite_cwd_relative_argument(pathname, rewritten, sizeof(rewritten)), mode);
}

int fstat(int fd, struct stat *cstat) { return orig_fstat(fd, cstat); }

int faccessat(int dirfd, const char *pathname, int mode, int flags) {
    char rewritten[PATH_MAX];
    const char *target = pathname;
    int call_dirfd = dirfd;

    if (pathname != NULL && pathname[0] == '/') {
        target = rewrite_path_argument(pathname, rewritten, sizeof(rewritten));
    } else if (rewrite_relative_path_from_dirfd(dirfd, pathname, rewritten, sizeof(rewritten))) {
        target = rewritten;
        call_dirfd = AT_FDCWD;
    }

    return orig_faccessat(call_dirfd, target, mode, flags);
}

int fchmodat(int dirfd, const char *pathname, mode_t mode, int flags) {
    char rewritten[PATH_MAX];
    const char *target = pathname;
    int call_dirfd = dirfd;

    if (pathname != NULL && pathname[0] == '/') {
        target = rewrite_path_argument(pathname, rewritten, sizeof(rewritten));
    } else if (rewrite_relative_path_from_dirfd(dirfd, pathname, rewritten, sizeof(rewritten))) {
        target = rewritten;
        call_dirfd = AT_FDCWD;
    }

    return orig_fchmodat(call_dirfd, target, mode, flags);
}

int link(const char *oldpath, const char *newpath) {
    char old_rewritten[PATH_MAX];
    char new_rewritten[PATH_MAX];
    return orig_link(rewrite_cwd_relative_argument(oldpath, old_rewritten, sizeof(old_rewritten)),
                     rewrite_cwd_relative_argument(newpath, new_rewritten, sizeof(new_rewritten)));
}

int linkat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags) {
    char old_rewritten[PATH_MAX];
    char new_rewritten[PATH_MAX];
    const char *effective_oldpath = rewrite_at_argument(&olddirfd, oldpath, old_rewritten,
                                                        sizeof(old_rewritten));
    const char *effective_newpath = rewrite_at_argument(&newdirfd, newpath, new_rewritten,
                                                        sizeof(new_rewritten));

    return orig_linkat(olddirfd, effective_oldpath, newdirfd, effective_newpath, flags);
}

off_t lseek(int fd, off_t offset, int whence) { return orig_lseek(fd, offset, whence); }

int mkdir(const char *pathname, mode_t mode) {
    char rewritten[PATH_MAX];
    return orig_mkdir(rewrite_path_argument(pathname, rewritten, sizeof(rewritten)), mode);
}

int mkdirat(int dirfd, const char *pathname, mode_t mode) {
    char rewritten[PATH_MAX];
    const char *target = pathname;
    int call_dirfd = dirfd;

    if (pathname != NULL && pathname[0] == '/') {
        target = rewrite_path_argument(pathname, rewritten, sizeof(rewritten));
    } else if (rewrite_relative_path_from_dirfd(dirfd, pathname, rewritten, sizeof(rewritten))) {
        target = rewritten;
        call_dirfd = AT_FDCWD;
    }

    return orig_mkdirat(call_dirfd, target, mode);
}

int open(const char *pathname, int flags, ...) {
    mode_t mode = 0;

    if (flags & (O_CREAT | O_TMPFILE)) {
        va_list args;
        va_start(args, flags);
        mode = (mode_t)va_arg(args, int);
        va_end(args);
    }

    {
        char rewritten[PATH_MAX];
        return orig_open(rewrite_path_argument(pathname, rewritten, sizeof(rewritten)), flags, mode);
    }
}

int openat(int dirfd, const char *pathname, int flags, ...) {
    mode_t mode = 0;

    if (flags & (O_CREAT | O_TMPFILE)) {
        va_list args;
        va_start(args, flags);
        mode = (mode_t)va_arg(args, int);
        va_end(args);
    }

    {
        char rewritten[PATH_MAX];
        const char *target = pathname;
        int call_dirfd = dirfd;

        if (pathname != NULL && pathname[0] == '/') {
            target = rewrite_path_argument(pathname, rewritten, sizeof(rewritten));
        } else if (rewrite_relative_path_from_dirfd(dirfd, pathname, rewritten,
                                                    sizeof(rewritten))) {
            target = rewritten;
            call_dirfd = AT_FDCWD;
        }

        return orig_openat(call_dirfd, target, flags, mode);
    }
}

int openat2(int dirfd, const char *pathname, const struct open_how *how, size_t size) {
    char rewritten[PATH_MAX];
    const char *target = pathname;
    int call_dirfd = dirfd;

    if (pathname != NULL && pathname[0] == '/') {
        target = rewrite_path_argument(pathname, rewritten, sizeof(rewritten));
    } else if (rewrite_relative_path_from_dirfd(dirfd, pathname, rewritten, sizeof(rewritten))) {
        target = rewritten;
        call_dirfd = AT_FDCWD;
    }

    return orig_openat2(call_dirfd, target, how, size);
}

char *getcwd(char *buf, size_t size) {
    char rewritten[PATH_MAX];
    char *result;

    result = orig_getcwd(buf, size);
    if (result == NULL) {
        return NULL;
    }
    if (!get_rewrite_policy()->enabled) {
        return result;
    }
    if (substitute_prefix_reverse(buf, rewritten, size) != 0) {
        return result;
    }
    if (strlen(rewritten) + 1 > size) {
        return result;
    }
    memcpy(buf, rewritten, strlen(rewritten) + 1);
    return buf;
}

ssize_t pread(int fd, void *buf, size_t count, off_t offset) {
    return orig_pread(fd, buf, count, offset);
}

ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset) {
    return orig_pwrite(fd, buf, count, offset);
}

ssize_t read(int fd, void *buf, size_t count) { return orig_read(fd, buf, count); }

ssize_t readlink(const char *pathname, char *buf, size_t bufsiz) {
    char rewritten[PATH_MAX];
    return orig_readlink(rewrite_path_argument(pathname, rewritten, sizeof(rewritten)), buf, bufsiz);
}

ssize_t readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz) {
    char rewritten[PATH_MAX];
    const char *target = pathname;
    int call_dirfd = dirfd;

    if (pathname != NULL && pathname[0] == '/') {
        target = rewrite_path_argument(pathname, rewritten, sizeof(rewritten));
    } else if (rewrite_relative_path_from_dirfd(dirfd, pathname, rewritten, sizeof(rewritten))) {
        target = rewritten;
        call_dirfd = AT_FDCWD;
    }

    return orig_readlinkat(call_dirfd, target, buf, bufsiz);
}

int rename(const char *oldpath, const char *newpath) {
    char old_rewritten[PATH_MAX];
    char new_rewritten[PATH_MAX];
    return orig_rename(rewrite_cwd_relative_argument(oldpath, old_rewritten, sizeof(old_rewritten)),
                       rewrite_cwd_relative_argument(newpath, new_rewritten, sizeof(new_rewritten)));
}

int renameat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath) {
    char old_rewritten[PATH_MAX];
    char new_rewritten[PATH_MAX];
    const char *effective_oldpath = rewrite_at_argument(&olddirfd, oldpath, old_rewritten,
                                                        sizeof(old_rewritten));
    const char *effective_newpath = rewrite_at_argument(&newdirfd, newpath, new_rewritten,
                                                        sizeof(new_rewritten));

    return orig_renameat(olddirfd, effective_oldpath, newdirfd, effective_newpath);
}

int renameat2(int olddirfd, const char *oldpath, int newdirfd, const char *newpath,
              unsigned int flags) {
    char old_rewritten[PATH_MAX];
    char new_rewritten[PATH_MAX];
    const char *effective_oldpath = rewrite_at_argument(&olddirfd, oldpath, old_rewritten,
                                                        sizeof(old_rewritten));
    const char *effective_newpath = rewrite_at_argument(&newdirfd, newpath, new_rewritten,
                                                        sizeof(new_rewritten));

    return orig_renameat2(olddirfd, effective_oldpath, newdirfd, effective_newpath, flags);
}

int lstat(const char *pathname, struct stat *cstat) {
    char rewritten[PATH_MAX];
    return orig_lstat(rewrite_path_argument(pathname, rewritten, sizeof(rewritten)), cstat);
}

int rmdir(const char *pathname) {
    char rewritten[PATH_MAX];
    return orig_rmdir(rewrite_path_argument(pathname, rewritten, sizeof(rewritten)));
}

int stat(const char *pathname, struct stat *cstat) {
    char rewritten[PATH_MAX];
    return orig_stat(rewrite_path_argument(pathname, rewritten, sizeof(rewritten)), cstat);
}

int statx(int dirfd, const char *pathname, int flags, unsigned int mask, struct statx *statxbuf) {
    char rewritten[PATH_MAX];
    const char *target = pathname;
    int call_dirfd = dirfd;

    if (pathname != NULL && pathname[0] == '/') {
        target = rewrite_path_argument(pathname, rewritten, sizeof(rewritten));
    } else if (rewrite_relative_path_from_dirfd(dirfd, pathname, rewritten, sizeof(rewritten))) {
        target = rewritten;
        call_dirfd = AT_FDCWD;
    }

    return orig_statx(call_dirfd, target, flags, mask, statxbuf);
}

int statfs(const char *path, struct statvfs *buf) {
    char rewritten[PATH_MAX];
    return orig_statfs(rewrite_cwd_relative_argument(path, rewritten, sizeof(rewritten)), buf);
}

int symlink(const char *target, const char *linkpath) {
    char target_rewritten[PATH_MAX];
    char link_rewritten[PATH_MAX];
    const char *effective_target = target;

    if (target != NULL && target[0] == '/') {
        effective_target = rewrite_path_argument(target, target_rewritten, sizeof(target_rewritten));
    }
    return orig_symlink(effective_target,
                        rewrite_cwd_relative_argument(linkpath, link_rewritten, sizeof(link_rewritten)));
}

int symlinkat(const char *target, int newdirfd, const char *linkpath) {
    char target_rewritten[PATH_MAX];
    char link_rewritten[PATH_MAX];
    const char *effective_target = target;
    const char *effective_linkpath = rewrite_at_argument(&newdirfd, linkpath, link_rewritten,
                                                         sizeof(link_rewritten));

    if (target != NULL && target[0] == '/') {
        effective_target = rewrite_path_argument(target, target_rewritten, sizeof(target_rewritten));
    }

    return orig_symlinkat(effective_target, newdirfd, effective_linkpath);
}

int truncate(const char *pathname, off_t length) {
    char rewritten[PATH_MAX];
    return orig_truncate(rewrite_path_argument(pathname, rewritten, sizeof(rewritten)), length);
}

int unlink(const char *pathname) {
    char rewritten[PATH_MAX];
    return orig_unlink(rewrite_path_argument(pathname, rewritten, sizeof(rewritten)));
}

int unlinkat(int dirfd, const char *pathname, int flags) {
    char rewritten[PATH_MAX];
    const char *target = pathname;
    int call_dirfd = dirfd;

    if (pathname != NULL && pathname[0] == '/') {
        target = rewrite_path_argument(pathname, rewritten, sizeof(rewritten));
    } else if (rewrite_relative_path_from_dirfd(dirfd, pathname, rewritten, sizeof(rewritten))) {
        target = rewritten;
        call_dirfd = AT_FDCWD;
    }

    return orig_unlinkat(call_dirfd, target, flags);
}

int utimensat(int dirfd, const char *pathname, const struct timespec times[2], int flags) {
    char rewritten[PATH_MAX];
    const char *target = pathname;
    int call_dirfd = dirfd;

    if (pathname != NULL && pathname[0] == '/') {
        target = rewrite_path_argument(pathname, rewritten, sizeof(rewritten));
    } else if (rewrite_relative_path_from_dirfd(dirfd, pathname, rewritten, sizeof(rewritten))) {
        target = rewritten;
        call_dirfd = AT_FDCWD;
    }

    return orig_utimensat(call_dirfd, target, times, flags);
}

ssize_t write(int fd, const void *buf, size_t count) { return orig_write(fd, buf, count); }
