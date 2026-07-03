#define _GNU_SOURCE

#ifndef __linux__

#include <stdio.h>

int main(void) {
    puts("linux-only smoke fixture");
    return 0;
}

#else

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <unistd.h>

static const char payload[] = "ld_preload smoke payload";

static void die(const char *message) {
    fprintf(stderr, "%s: %s\n", message, strerror(errno));
    exit(1);
}

static void fail_message(const char *message) {
    fprintf(stderr, "%s\n", message);
    exit(1);
}

static void make_path(char *dst, size_t dst_len, const char *dir, const char *name) {
    if (snprintf(dst, dst_len, "%s/%s", dir, name) >= (int)dst_len) {
        fail_message("path too long");
    }
}

static void expect_errno_int(const char *label, int actual, int expected) {
    if (actual != expected) {
        fprintf(stderr, "%s: expected errno %d, got %d\n", label, expected, actual);
        exit(1);
    }
}

static void expect_size(const char *label, off_t actual, off_t expected) {
    if (actual != expected) {
        fprintf(stderr, "%s: expected size %lld, got %lld\n", label, (long long)expected,
                (long long)actual);
        exit(1);
    }
}

static void expect_bytes(const char *label, const char *actual, const char *expected, size_t len) {
    if (memcmp(actual, expected, len) != 0) {
        fprintf(stderr, "%s: payload mismatch\n", label);
        exit(1);
    }
}

static int is_optional_xattr_errno(int err) {
    return err == ENOTSUP || err == EOPNOTSUPP || err == EPERM;
}

static void create_scratch_dir(char *dir_buf, size_t dir_buf_len) {
    char template[] = "/tmp/ld_preload_smoke.XXXXXX";
    char *dir = mkdtemp(template);
    if (dir == NULL) {
        die("mkdtemp");
    }
    if (snprintf(dir_buf, dir_buf_len, "%s", dir) >= (int)dir_buf_len) {
        fail_message("scratch dir too long");
    }
}

static void cleanup_path(const char *path) {
    if (unlink(path) != 0 && errno != ENOENT) {
        die("unlink cleanup");
    }
}

static void cleanup_dir(const char *path) {
    if (rmdir(path) != 0 && errno != ENOENT) {
        die("rmdir cleanup");
    }
}

static void run_basic(void) {
    char dir[PATH_MAX];
    char file_path[PATH_MAX];
    char buf[128];
    struct stat st;
    int fd;
    int dirfd;
    int fd_at;
    ssize_t n;

    create_scratch_dir(dir, sizeof(dir));
    make_path(file_path, sizeof(file_path), dir, "basic.txt");

    fd = creat(file_path, 0644);
    if (fd < 0) {
        die("creat");
    }

    n = write(fd, payload, sizeof(payload) - 1);
    if (n != (ssize_t)(sizeof(payload) - 1)) {
        die("write");
    }

    if (close(fd) != 0) {
        die("close after creat");
    }

    if (access(file_path, R_OK | W_OK) != 0) {
        die("access");
    }
    if (euidaccess(file_path, R_OK) != 0) {
        die("euidaccess");
    }

    fd = open(file_path, O_RDWR);
    if (fd < 0) {
        die("open");
    }

    if (lseek(fd, 0, SEEK_SET) != 0) {
        die("lseek rewind");
    }

    memset(buf, 0, sizeof(buf));
    n = read(fd, buf, sizeof(buf));
    if (n != (ssize_t)(sizeof(payload) - 1)) {
        die("read after lseek");
    }
    expect_bytes("read after lseek", buf, payload, sizeof(payload) - 1);

    if (pwrite(fd, "PRE", 3, 0) != 3) {
        die("pwrite");
    }
    memset(buf, 0, sizeof(buf));
    if (pread(fd, buf, sizeof(payload) - 1, 0) != (ssize_t)(sizeof(payload) - 1)) {
        die("pread");
    }
    expect_bytes("pread prefix", buf, "PRE", 3);

    if (fstat(fd, &st) != 0) {
        die("fstat");
    }
    expect_size("fstat size", st.st_size, sizeof(payload) - 1);

    dirfd = open(dir, O_RDONLY | O_DIRECTORY);
    if (dirfd < 0) {
        die("open dir");
    }

    if (faccessat(dirfd, "basic.txt", R_OK, 0) != 0) {
        die("faccessat");
    }

    fd_at = openat(dirfd, "basic.txt", O_RDONLY);
    if (fd_at < 0) {
        die("openat");
    }
    if (close(fd_at) != 0) {
        die("close openat");
    }

    if (stat(file_path, &st) != 0) {
        die("stat");
    }
    expect_size("stat size", st.st_size, sizeof(payload) - 1);

    if (close(fd) != 0) {
        die("close");
    }

    if (truncate(file_path, 7) != 0) {
        die("truncate");
    }
    if (stat(file_path, &st) != 0) {
        die("stat after truncate");
    }
    expect_size("truncate size", st.st_size, 7);

    if (close(dirfd) != 0) {
        die("close dir");
    }

    cleanup_path(file_path);
    cleanup_dir(dir);
}

static void run_metadata(void) {
    char dir[PATH_MAX];
    char nested_dir[PATH_MAX];
    char file_path[PATH_MAX];
    char renamed_path[PATH_MAX];
    char symlink_path[PATH_MAX];
    char hardlink_path[PATH_MAX];
    char link_target[PATH_MAX];
    struct stat st;
    int fd;
    ssize_t n;

    create_scratch_dir(dir, sizeof(dir));
    make_path(nested_dir, sizeof(nested_dir), dir, "nested");
    make_path(file_path, sizeof(file_path), nested_dir, "source.txt");
    make_path(renamed_path, sizeof(renamed_path), nested_dir, "renamed.txt");
    make_path(symlink_path, sizeof(symlink_path), dir, "source.lnk");
    make_path(hardlink_path, sizeof(hardlink_path), dir, "source.hard");

    if (mkdir(nested_dir, 0755) != 0) {
        die("mkdir");
    }

    fd = creat(file_path, 0644);
    if (fd < 0) {
        die("creat metadata");
    }
    n = write(fd, payload, sizeof(payload) - 1);
    if (n != (ssize_t)(sizeof(payload) - 1)) {
        die("write metadata");
    }
    if (close(fd) != 0) {
        die("close metadata");
    }

    if (rename(file_path, renamed_path) != 0) {
        die("rename");
    }
    if (chmod(renamed_path, 0600) != 0) {
        die("chmod");
    }
    if (stat(renamed_path, &st) != 0) {
        die("stat renamed");
    }
    if ((st.st_mode & 0777) != 0600) {
        fail_message("chmod mode mismatch");
    }

    if (symlink(renamed_path, symlink_path) != 0) {
        die("symlink");
    }
    if (lstat(symlink_path, &st) != 0) {
        die("lstat symlink");
    }
    if (!S_ISLNK(st.st_mode)) {
        fail_message("lstat did not report symlink");
    }

    memset(link_target, 0, sizeof(link_target));
    n = readlink(symlink_path, link_target, sizeof(link_target) - 1);
    if (n < 0) {
        die("readlink");
    }
    link_target[n] = '\0';
    if (strcmp(link_target, renamed_path) != 0) {
        fail_message("readlink target mismatch");
    }

    if (link(renamed_path, hardlink_path) != 0) {
        die("link");
    }
    if (stat(hardlink_path, &st) != 0) {
        die("stat hardlink");
    }
    if (st.st_nlink < 2) {
        fail_message("hardlink count mismatch");
    }

    cleanup_path(hardlink_path);
    cleanup_path(symlink_path);
    cleanup_path(renamed_path);
    cleanup_dir(nested_dir);
    cleanup_dir(dir);
}

static void run_missing(void) {
    char dir[PATH_MAX];
    char missing_path[PATH_MAX];
    int dirfd;
    struct stat st;
    char buf[16];

    create_scratch_dir(dir, sizeof(dir));
    make_path(missing_path, sizeof(missing_path), dir, "missing.txt");

    errno = 0;
    if (access(missing_path, F_OK) != -1) {
        fail_message("access missing unexpectedly succeeded");
    }
    expect_errno_int("access missing", errno, ENOENT);

    errno = 0;
    if (open(missing_path, O_RDONLY) != -1) {
        fail_message("open missing unexpectedly succeeded");
    }
    expect_errno_int("open missing", errno, ENOENT);

    errno = 0;
    if (stat(missing_path, &st) != -1) {
        fail_message("stat missing unexpectedly succeeded");
    }
    expect_errno_int("stat missing", errno, ENOENT);

    errno = 0;
    if (lstat(missing_path, &st) != -1) {
        fail_message("lstat missing unexpectedly succeeded");
    }
    expect_errno_int("lstat missing", errno, ENOENT);

    errno = 0;
    if (readlink(missing_path, buf, sizeof(buf)) != -1) {
        fail_message("readlink missing unexpectedly succeeded");
    }
    expect_errno_int("readlink missing", errno, ENOENT);

    errno = 0;
    if (getxattr(missing_path, "user.ld_preload", buf, sizeof(buf)) != -1) {
        fail_message("getxattr missing unexpectedly succeeded");
    }
    expect_errno_int("getxattr missing", errno, ENOENT);

    dirfd = open(dir, O_RDONLY | O_DIRECTORY);
    if (dirfd < 0) {
        die("open dir missing");
    }

    errno = 0;
    if (openat(dirfd, "missing.txt", O_RDONLY) != -1) {
        fail_message("openat missing unexpectedly succeeded");
    }
    expect_errno_int("openat missing", errno, ENOENT);

    errno = 0;
    if (faccessat(dirfd, "missing.txt", R_OK, 0) != -1) {
        fail_message("faccessat missing unexpectedly succeeded");
    }
    expect_errno_int("faccessat missing", errno, ENOENT);

    if (close(dirfd) != 0) {
        die("close dir missing");
    }
    cleanup_dir(dir);
}

static void run_xattr(void) {
    char dir[PATH_MAX];
    char file_path[PATH_MAX];
    int fd;
    ssize_t n;
    char buf[64];
    const char *attr_name = "user.ld_preload";
    const char *attr_value = "xattr-value";

    create_scratch_dir(dir, sizeof(dir));
    make_path(file_path, sizeof(file_path), dir, "xattr.txt");

    fd = creat(file_path, 0644);
    if (fd < 0) {
        die("creat xattr");
    }
    n = write(fd, payload, sizeof(payload) - 1);
    if (n != (ssize_t)(sizeof(payload) - 1)) {
        die("write xattr");
    }

    if (setxattr(file_path, attr_name, attr_value, strlen(attr_value), 0) != 0) {
        if (is_optional_xattr_errno(errno)) {
            printf("SKIP xattr setup unsupported: %s\n", strerror(errno));
            close(fd);
            cleanup_path(file_path);
            cleanup_dir(dir);
            return;
        }
        die("setxattr");
    }

    memset(buf, 0, sizeof(buf));
    n = getxattr(file_path, attr_name, buf, sizeof(buf));
    if (n != (ssize_t)strlen(attr_value)) {
        die("getxattr");
    }
    expect_bytes("getxattr", buf, attr_value, strlen(attr_value));

    memset(buf, 0, sizeof(buf));
    n = fgetxattr(fd, attr_name, buf, sizeof(buf));
    if (n != (ssize_t)strlen(attr_value)) {
        die("fgetxattr");
    }
    expect_bytes("fgetxattr", buf, attr_value, strlen(attr_value));

    errno = 0;
    if (lgetxattr(file_path, attr_name, buf, sizeof(buf)) != (ssize_t)strlen(attr_value)) {
        die("lgetxattr regular file");
    }

    if (close(fd) != 0) {
        die("close xattr");
    }
    cleanup_path(file_path);
    cleanup_dir(dir);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <basic|metadata|missing|xattr>\n", argv[0]);
        return 2;
    }

    if (strcmp(argv[1], "basic") == 0) {
        run_basic();
        return 0;
    }
    if (strcmp(argv[1], "metadata") == 0) {
        run_metadata();
        return 0;
    }
    if (strcmp(argv[1], "missing") == 0) {
        run_missing();
        return 0;
    }
    if (strcmp(argv[1], "xattr") == 0) {
        run_xattr();
        return 0;
    }

    fprintf(stderr, "unknown scenario: %s\n", argv[1]);
    return 2;
}

#endif
