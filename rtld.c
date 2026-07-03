//go:build linux
// +build linux

#define _GNU_SOURCE

#include "rtld.h"

#include <dlfcn.h>
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>

typedef int (*xstat_fn)(int, const char *, struct stat *);
typedef int (*fxstat_fn)(int, int, struct stat *);
typedef int (*fxstatat_fn)(int, int, const char *, struct stat *, int);
typedef int (*access_fn)(const char *, int);
typedef int (*chmod_fn)(const char *, mode_t);
typedef int (*chown_fn)(const char *, uid_t, gid_t);
typedef int (*close_fn)(int);
typedef int (*creat_fn)(const char *, mode_t);
typedef int (*euidaccess_fn)(const char *, int);
typedef int (*faccessat_fn)(int, const char *, int, int);
typedef ssize_t (*xattr_fn)(const char *, const char *, void *, size_t);
typedef ssize_t (*fxattr_fn)(int, const char *, void *, size_t);
typedef int (*fstat_fn)(int, struct stat *);
typedef int (*link_fn)(const char *, const char *);
typedef off_t (*lseek_fn)(int, off_t, int);
typedef int (*mkdir_fn)(const char *, mode_t);
typedef int (*mknod_fn)(const char *, mode_t, dev_t);
typedef int (*mknodat_fn)(int, const char *, mode_t, dev_t);
typedef int (*open_fn)(const char *, int, ...);
typedef int (*openat_fn)(int, const char *, int, ...);
typedef ssize_t (*pread_fn)(int, void *, size_t, off_t);
typedef ssize_t (*pwrite_fn)(int, const void *, size_t, off_t);
typedef ssize_t (*read_fn)(int, void *, size_t);
typedef ssize_t (*readlink_fn)(const char *, char *, size_t);
typedef int (*rename_fn)(const char *, const char *);
typedef int (*rmdir_fn)(const char *);
typedef int (*symlink_fn)(const char *, const char *);
typedef int (*truncate_fn)(const char *, off_t);
typedef int (*unlink_fn)(const char *);
typedef ssize_t (*write_fn)(int, const void *, size_t);

static pthread_once_t resolve_once = PTHREAD_ONCE_INIT;

static fxstat_fn real___fxstat;
static fxstatat_fn real___fxstatat;
static xstat_fn real___lxstat;
static xstat_fn real___xstat;
static access_fn real_access;
static chmod_fn real_chmod;
static chown_fn real_chown;
static close_fn real_close;
static creat_fn real_creat;
static euidaccess_fn real_euidaccess;
static faccessat_fn real_faccessat;
static fxattr_fn real_fgetxattr;
static fstat_fn real_fstat;
static xattr_fn real_getxattr;
static xattr_fn real_lgetxattr;
static link_fn real_link;
static lseek_fn real_lseek;
static mkdir_fn real_mkdir;
static mknod_fn real_mknod;
static mknodat_fn real_mknodat;
static open_fn real_open;
static openat_fn real_openat;
static pread_fn real_pread;
static pwrite_fn real_pwrite;
static read_fn real_read;
static readlink_fn real_readlink;
static rename_fn real_rename;
static rmdir_fn real_rmdir;
static symlink_fn real_symlink;
static truncate_fn real_truncate;
static unlink_fn real_unlink;
static write_fn real_write;

static void resolve_symbols(void) {
    real___fxstat = (fxstat_fn)dlsym(RTLD_NEXT, "__fxstat");
    real___fxstatat = (fxstatat_fn)dlsym(RTLD_NEXT, "__fxstatat");
    real___lxstat = (xstat_fn)dlsym(RTLD_NEXT, "__lxstat");
    real___xstat = (xstat_fn)dlsym(RTLD_NEXT, "__xstat");
    real_access = (access_fn)dlsym(RTLD_NEXT, "access");
    real_chmod = (chmod_fn)dlsym(RTLD_NEXT, "chmod");
    real_chown = (chown_fn)dlsym(RTLD_NEXT, "chown");
    real_close = (close_fn)dlsym(RTLD_NEXT, "close");
    real_creat = (creat_fn)dlsym(RTLD_NEXT, "creat");
    real_euidaccess = (euidaccess_fn)dlsym(RTLD_NEXT, "euidaccess");
    real_faccessat = (faccessat_fn)dlsym(RTLD_NEXT, "faccessat");
    real_fgetxattr = (fxattr_fn)dlsym(RTLD_NEXT, "fgetxattr");
    real_fstat = (fstat_fn)dlsym(RTLD_NEXT, "fstat");
    real_getxattr = (xattr_fn)dlsym(RTLD_NEXT, "getxattr");
    real_lgetxattr = (xattr_fn)dlsym(RTLD_NEXT, "lgetxattr");
    real_link = (link_fn)dlsym(RTLD_NEXT, "link");
    real_lseek = (lseek_fn)dlsym(RTLD_NEXT, "lseek");
    real_mkdir = (mkdir_fn)dlsym(RTLD_NEXT, "mkdir");
    real_mknod = (mknod_fn)dlsym(RTLD_NEXT, "mknod");
    real_mknodat = (mknodat_fn)dlsym(RTLD_NEXT, "mknodat");
    real_open = (open_fn)dlsym(RTLD_NEXT, "open");
    real_openat = (openat_fn)dlsym(RTLD_NEXT, "openat");
    real_pread = (pread_fn)dlsym(RTLD_NEXT, "pread");
    real_pwrite = (pwrite_fn)dlsym(RTLD_NEXT, "pwrite");
    real_read = (read_fn)dlsym(RTLD_NEXT, "read");
    real_readlink = (readlink_fn)dlsym(RTLD_NEXT, "readlink");
    real_rename = (rename_fn)dlsym(RTLD_NEXT, "rename");
    real_rmdir = (rmdir_fn)dlsym(RTLD_NEXT, "rmdir");
    real_symlink = (symlink_fn)dlsym(RTLD_NEXT, "symlink");
    real_truncate = (truncate_fn)dlsym(RTLD_NEXT, "truncate");
    real_unlink = (unlink_fn)dlsym(RTLD_NEXT, "unlink");
    real_write = (write_fn)dlsym(RTLD_NEXT, "write");
}

static void ensure_symbols(void) {
    pthread_once(&resolve_once, resolve_symbols);
}

#define REQUIRE_SYMBOL(sym) \
    do {                    \
        ensure_symbols();   \
        if ((sym) == NULL) {\
            errno = ENOSYS; \
            return -1;      \
        }                   \
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

int orig_chmod(const char *pathname, mode_t mode) {
    REQUIRE_SYMBOL(real_chmod);
    return real_chmod(pathname, mode);
}

int orig_chown(const char *pathname, uid_t owner, gid_t group) {
    REQUIRE_SYMBOL(real_chown);
    return real_chown(pathname, owner, group);
}

int orig_close(int fd) {
    REQUIRE_SYMBOL(real_close);
    return real_close(fd);
}

int orig_creat(const char *pathname, mode_t mode) {
    REQUIRE_SYMBOL(real_creat);
    return real_creat(pathname, mode);
}

int orig_euidaccess(const char *pathname, int mode) {
    REQUIRE_SYMBOL(real_euidaccess);
    return real_euidaccess(pathname, mode);
}

int orig_faccessat(int dirfd, const char *pathname, int mode, int flags) {
    REQUIRE_SYMBOL(real_faccessat);
    return real_faccessat(dirfd, pathname, mode, flags);
}

ssize_t orig_fgetxattr(int fd, const char *name, void *value, size_t size) {
    REQUIRE_SYMBOL(real_fgetxattr);
    return real_fgetxattr(fd, name, value, size);
}

int orig_fstat(int fd, struct stat *cstat) {
    REQUIRE_SYMBOL(real_fstat);
    return real_fstat(fd, cstat);
}

ssize_t orig_getxattr(const char *pathname, const char *name, void *value, size_t size) {
    REQUIRE_SYMBOL(real_getxattr);
    return real_getxattr(pathname, name, value, size);
}

ssize_t orig_lgetxattr(const char *pathname, const char *name, void *value, size_t size) {
    REQUIRE_SYMBOL(real_lgetxattr);
    return real_lgetxattr(pathname, name, value, size);
}

int orig_link(const char *oldpath, const char *newpath) {
    REQUIRE_SYMBOL(real_link);
    return real_link(oldpath, newpath);
}

off_t orig_lseek(int fd, off_t offset, int whence) {
    REQUIRE_SYMBOL(real_lseek);
    return real_lseek(fd, offset, whence);
}

int orig_mkdir(const char *pathname, mode_t mode) {
    REQUIRE_SYMBOL(real_mkdir);
    return real_mkdir(pathname, mode);
}

int orig_mknod(const char *pathname, mode_t mode, dev_t dev) {
    REQUIRE_SYMBOL(real_mknod);
    return real_mknod(pathname, mode, dev);
}

int orig_mknodat(int dirfd, const char *pathname, mode_t mode, dev_t dev) {
    REQUIRE_SYMBOL(real_mknodat);
    return real_mknodat(dirfd, pathname, mode, dev);
}

int orig_open(const char *pathname, int flags, mode_t mode) {
    REQUIRE_SYMBOL(real_open);
    return real_open(pathname, flags, mode);
}

int orig_openat(int dirfd, const char *pathname, int flags, mode_t mode) {
    REQUIRE_SYMBOL(real_openat);
    return real_openat(dirfd, pathname, flags, mode);
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

int orig_rename(const char *oldpath, const char *newpath) {
    REQUIRE_SYMBOL(real_rename);
    return real_rename(oldpath, newpath);
}

int orig_rmdir(const char *pathname) {
    REQUIRE_SYMBOL(real_rmdir);
    return real_rmdir(pathname);
}

int orig_symlink(const char *target, const char *linkpath) {
    REQUIRE_SYMBOL(real_symlink);
    return real_symlink(target, linkpath);
}

int orig_truncate(const char *pathname, off_t length) {
    REQUIRE_SYMBOL(real_truncate);
    return real_truncate(pathname, length);
}

int orig_unlink(const char *pathname) {
    REQUIRE_SYMBOL(real_unlink);
    return real_unlink(pathname);
}

ssize_t orig_write(int fd, const void *buf, size_t count) {
    REQUIRE_SYMBOL(real_write);
    return real_write(fd, buf, count);
}

int __fxstat(int ver, int fd, struct stat *cstat) {
    return orig___fxstat(ver, fd, cstat);
}

int __fxstatat(int ver, int dirfd, const char *pathname, struct stat *cstat, int flags) {
    return orig___fxstatat(ver, dirfd, pathname, cstat, flags);
}

int __lxstat(int ver, const char *pathname, struct stat *cstat) {
    return orig___lxstat(ver, pathname, cstat);
}

int __xstat(int ver, const char *pathname, struct stat *cstat) {
    return orig___xstat(ver, pathname, cstat);
}

int access(const char *pathname, int mode) {
    return orig_access(pathname, mode);
}

int chmod(const char *pathname, mode_t mode) {
    return orig_chmod(pathname, mode);
}

int chown(const char *pathname, uid_t owner, gid_t group) {
    return orig_chown(pathname, owner, group);
}

int close(int fd) {
    return orig_close(fd);
}

int creat(const char *pathname, mode_t mode) {
    return orig_creat(pathname, mode);
}

int euidaccess(const char *pathname, int mode) {
    return orig_euidaccess(pathname, mode);
}

int faccessat(int dirfd, const char *pathname, int mode, int flags) {
    return orig_faccessat(dirfd, pathname, mode, flags);
}

ssize_t fgetxattr(int fd, const char *name, void *value, size_t size) {
    return orig_fgetxattr(fd, name, value, size);
}

int fstat(int fd, struct stat *cstat) {
    return orig_fstat(fd, cstat);
}

ssize_t getxattr(const char *pathname, const char *name, void *value, size_t size) {
    return orig_getxattr(pathname, name, value, size);
}

ssize_t lgetxattr(const char *pathname, const char *name, void *value, size_t size) {
    return orig_lgetxattr(pathname, name, value, size);
}

int link(const char *oldpath, const char *newpath) {
    return orig_link(oldpath, newpath);
}

off_t lseek(int fd, off_t offset, int whence) {
    return orig_lseek(fd, offset, whence);
}

int mkdir(const char *pathname, mode_t mode) {
    return orig_mkdir(pathname, mode);
}

int mknod(const char *pathname, mode_t mode, dev_t dev) {
    return orig_mknod(pathname, mode, dev);
}

int mknodat(int dirfd, const char *pathname, mode_t mode, dev_t dev) {
    return orig_mknodat(dirfd, pathname, mode, dev);
}

int open(const char *pathname, int flags, ...) {
    mode_t mode = 0;

    if (flags & (O_CREAT | O_TMPFILE)) {
        va_list args;
        va_start(args, flags);
        mode = (mode_t)va_arg(args, int);
        va_end(args);
    }

    return orig_open(pathname, flags, mode);
}

int openat(int dirfd, const char *pathname, int flags, ...) {
    mode_t mode = 0;

    if (flags & (O_CREAT | O_TMPFILE)) {
        va_list args;
        va_start(args, flags);
        mode = (mode_t)va_arg(args, int);
        va_end(args);
    }

    return orig_openat(dirfd, pathname, flags, mode);
}

ssize_t pread(int fd, void *buf, size_t count, off_t offset) {
    return orig_pread(fd, buf, count, offset);
}

ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset) {
    return orig_pwrite(fd, buf, count, offset);
}

ssize_t read(int fd, void *buf, size_t count) {
    return orig_read(fd, buf, count);
}

ssize_t readlink(const char *pathname, char *buf, size_t bufsiz) {
    return orig_readlink(pathname, buf, bufsiz);
}

int rename(const char *oldpath, const char *newpath) {
    return orig_rename(oldpath, newpath);
}

int rmdir(const char *pathname) {
    return orig_rmdir(pathname);
}

int symlink(const char *target, const char *linkpath) {
    return orig_symlink(target, linkpath);
}

int truncate(const char *pathname, off_t length) {
    return orig_truncate(pathname, length);
}

int unlink(const char *pathname) {
    return orig_unlink(pathname);
}

ssize_t write(int fd, const void *buf, size_t count) {
    return orig_write(fd, buf, count);
}
