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
#include <linux/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <sys/wait.h>
#include <unistd.h>

static const char payload[] = "ld_preload smoke payload";
static const char *fixture_argv0;
static const char payload_fd[] = "fd semantics payload";

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

static void write_file_exact(const char *path, const char *content) {
    int fd = creat(path, 0644);
    size_t len = strlen(content);

    if (fd < 0) {
        die("creat helper");
    }
    if (write(fd, content, len) != (ssize_t)len) {
        die("write helper");
    }
    if (close(fd) != 0) {
        die("close helper");
    }
}

static void run_passthrough(const char *source_root, const char *target_root) {
    char source_dir[PATH_MAX];
    char source_file[PATH_MAX];
    char target_dir[PATH_MAX];
    char target_file[PATH_MAX];
    char buf[128];
    struct stat st;
    int dirfd;
    int fd;
    ssize_t n;
    const char *source_payload = "source payload";
    const char *target_payload = "target payload";

    make_path(source_dir, sizeof(source_dir), source_root, "tree");
    make_path(source_file, sizeof(source_file), source_dir, "data.txt");
    make_path(target_dir, sizeof(target_dir), target_root, "tree");
    make_path(target_file, sizeof(target_file), target_dir, "data.txt");

    if (mkdir(source_dir, 0755) != 0 && errno != EEXIST) {
        die("mkdir passthrough source");
    }
    if (mkdir(target_dir, 0755) != 0 && errno != EEXIST) {
        die("mkdir passthrough target");
    }

    write_file_exact(source_file, source_payload);
    write_file_exact(target_file, target_payload);

    fd = open(source_file, O_RDONLY);
    if (fd < 0) {
        die("open passthrough source");
    }
    memset(buf, 0, sizeof(buf));
    n = read(fd, buf, sizeof(buf));
    if (n != (ssize_t)strlen(source_payload)) {
        die("read passthrough source");
    }
    expect_bytes("passthrough open payload", buf, source_payload, strlen(source_payload));
    if (close(fd) != 0) {
        die("close passthrough source");
    }

    dirfd = open(source_dir, O_RDONLY | O_DIRECTORY);
    if (dirfd < 0) {
        die("open passthrough dir");
    }
    fd = openat(dirfd, "data.txt", O_RDONLY);
    if (fd < 0) {
        die("openat passthrough source");
    }
    memset(buf, 0, sizeof(buf));
    n = read(fd, buf, sizeof(buf));
    if (n != (ssize_t)strlen(source_payload)) {
        die("read passthrough openat");
    }
    expect_bytes("passthrough openat payload", buf, source_payload, strlen(source_payload));
    if (close(fd) != 0) {
        die("close passthrough openat");
    }

    if (stat(source_file, &st) != 0) {
        die("stat passthrough source");
    }
    expect_size("passthrough stat size", st.st_size, (off_t)strlen(source_payload));

    if (stat(target_file, &st) != 0) {
        die("stat passthrough target");
    }
    expect_size("passthrough target size", st.st_size, (off_t)strlen(target_payload));

    if (close(dirfd) != 0) {
        die("close passthrough dir");
    }
}

static void run_failopen(void) {
    char cwd_template[] = "/tmp/ld_preload_failopen.XXXXXX";
    char *tmp_dir;
    char missing_buf[32];
    struct stat st;
    struct statx stx;
    int saved_cwd_fd;
    int dirfd;
    int fd;

    errno = 0;
    if (openat(-1, "missing.txt", O_RDONLY) != -1) {
        fail_message("openat invalid dirfd unexpectedly succeeded");
    }
    expect_errno_int("openat invalid dirfd", errno, EBADF);

    errno = 0;
    if (faccessat(-1, "missing.txt", R_OK, 0) != -1) {
        fail_message("faccessat invalid dirfd unexpectedly succeeded");
    }
    expect_errno_int("faccessat invalid dirfd", errno, EBADF);

    errno = 0;
    if (readlinkat(-1, "missing.txt", missing_buf, sizeof(missing_buf)) != -1) {
        fail_message("readlinkat invalid dirfd unexpectedly succeeded");
    }
    expect_errno_int("readlinkat invalid dirfd", errno, EBADF);

    errno = 0;
    if (statx(-1, "missing.txt", 0, STATX_SIZE, &stx) != -1) {
        fail_message("statx invalid dirfd unexpectedly succeeded");
    }
    expect_errno_int("statx invalid dirfd", errno, EBADF);

    tmp_dir = mkdtemp(cwd_template);
    if (tmp_dir == NULL) {
        die("mkdtemp failopen");
    }

    saved_cwd_fd = open(".", O_RDONLY | O_DIRECTORY);
    if (saved_cwd_fd < 0) {
        die("open saved cwd");
    }
    dirfd = open(tmp_dir, O_RDONLY | O_DIRECTORY);
    if (dirfd < 0) {
        die("open failopen dir");
    }
    if (close(dirfd) != 0) {
        die("close failopen dir");
    }

    errno = 0;
    if (openat(dirfd, "missing.txt", O_RDONLY) != -1) {
        fail_message("openat closed dirfd unexpectedly succeeded");
    }
    expect_errno_int("openat closed dirfd", errno, EBADF);

    if (chdir(tmp_dir) != 0) {
        die("chdir failopen dir");
    }
    if (rmdir(tmp_dir) != 0) {
        die("rmdir failopen dir");
    }

    errno = 0;
    fd = open("missing.txt", O_RDONLY);
    if (fd != -1) {
        fail_message("open missing in deleted cwd unexpectedly succeeded");
    }
    expect_errno_int("open missing in deleted cwd", errno, ENOENT);

    errno = 0;
    if (stat("missing.txt", &st) != -1) {
        fail_message("stat missing in deleted cwd unexpectedly succeeded");
    }
    expect_errno_int("stat missing in deleted cwd", errno, ENOENT);

    if (fchdir(saved_cwd_fd) != 0) {
        die("restore cwd");
    }
    if (close(saved_cwd_fd) != 0) {
        die("close saved cwd");
    }
}

static void run_fd_semantics(const char *source_root, const char *target_root) {
    char target_dir[PATH_MAX];
    char target_file[PATH_MAX];
    char source_file[PATH_MAX];
    char payload_buf[] = "fd semantics payload";
    char read_buf[64];
    pid_t child_pid;
    int status;
    int shared_fd;
    int keep_fd;
    int cloexec_fd;
    ssize_t n;
    int child_closed_fd;

    make_path(target_dir, sizeof(target_dir), target_root, "tree");
    make_path(target_file, sizeof(target_file), target_dir, "shared.txt");
    make_path(source_file, sizeof(source_file), source_root, "tree/shared.txt");

    if (mkdir(target_dir, 0755) != 0 && errno != EEXIST) {
        die("mkdir fd target tree");
    }
    write_file_exact(target_file, payload_buf);

    shared_fd = open(source_file, O_RDONLY);
    if (shared_fd < 0) {
        die("open fd shared");
    }

    child_pid = fork();
    if (child_pid < 0) {
        die("fork inherit");
    }
    if (child_pid == 0) {
        n = read(shared_fd, read_buf, sizeof(read_buf));
        if (n != (ssize_t)strlen(payload_buf)) {
            _exit(1);
        }
        expect_bytes("fork-inherited read", read_buf, payload_buf, strlen(payload_buf));
        _exit(0);
    }
    if (close(shared_fd) != 0) {
        die("close shared fd");
    }
    if (waitpid(child_pid, &status, 0) != child_pid) {
        die("wait fork inherit");
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        die("fork-inherited child failed");
    }

    shared_fd = open(source_file, O_RDONLY);
    if (shared_fd < 0) {
        die("open fd fork-close");
    }
    child_pid = fork();
    if (child_pid < 0) {
        die("fork child close");
    }
    if (child_pid == 0) {
        child_closed_fd = close(shared_fd);
        _exit(child_closed_fd == 0 ? 0 : 1);
    }
    if (waitpid(child_pid, &status, 0) != child_pid) {
        die("wait fork child close");
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        die("fork child close failed");
    }
    memset(read_buf, 0, sizeof(read_buf));
    n = pread(shared_fd, read_buf, sizeof(read_buf), 0);
    if (n != (ssize_t)strlen(payload_buf)) {
        die("fork child close pread");
    }
    expect_bytes("fork child close pread", read_buf, payload_buf, strlen(payload_buf));

    keep_fd = open(source_file, O_RDONLY);
    if (keep_fd < 0) {
        die("open fd exec keep");
    }
    cloexec_fd = open(source_file, O_RDONLY | O_CLOEXEC);
    if (cloexec_fd < 0) {
        die("open fd exec cloexec");
    }

    child_pid = fork();
    if (child_pid < 0) {
        die("fork exec check");
    }
    if (child_pid == 0) {
        char keep_arg[32];
        char cloexec_arg[32];
        char *argv_exec[] = {(char *)fixture_argv0, "fd-exec-check", keep_arg, cloexec_arg, NULL};

        snprintf(keep_arg, sizeof(keep_arg), "%d", keep_fd);
        snprintf(cloexec_arg, sizeof(cloexec_arg), "%d", cloexec_fd);
        execv(fixture_argv0, argv_exec);
        _exit(1);
    }
    if (waitpid(child_pid, &status, 0) != child_pid) {
        die("wait exec check");
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        die("exec check child failed");
    }

    if (close(shared_fd) != 0) {
        die("close shared fd");
    }
    if (close(keep_fd) != 0) {
        die("close parent keep fd");
    }
    if (close(cloexec_fd) != 0) {
        die("close parent cloexec fd");
    }
}

static void run_fd_exec_check(int keep_fd, int cloexec_fd) {
    struct stat st;
    char read_buf[64];
    ssize_t n;
    size_t expected;

    if (fstat(keep_fd, &st) != 0) {
        die("fdexec keep fstat");
    }

    n = read(keep_fd, read_buf, sizeof(read_buf));
    expected = strlen(payload_fd);
    if (n != (ssize_t)expected) {
        die("fdexec keep read");
    }
    expect_bytes("fdexec keep payload", read_buf, payload_fd, expected);

    if (lseek(keep_fd, 0, SEEK_SET) != 0) {
        die("fdexec keep rewind");
    }

    errno = 0;
    if (fstat(cloexec_fd, &st) != -1 || errno != EBADF) {
        die("fdexec cloexec is still open");
    }
}

static void run_rewrite(const char *source_root, const char *target_root, const char *sibling_root) {
    char source_dir[PATH_MAX];
    char source_file[PATH_MAX];
    char source_renamed[PATH_MAX];
    char source_link[PATH_MAX];
    char source_abs_symlink[PATH_MAX];
    char source_rel_symlink[PATH_MAX];
    char source_abs_symlink_at[PATH_MAX];
    char source_rel_symlink_at[PATH_MAX];
    char source_missing[PATH_MAX];
    char source_guard[PATH_MAX];
    char source_created_dir_at[PATH_MAX];
    char source_remove_at[PATH_MAX];
    char target_dir[PATH_MAX];
    char target_file[PATH_MAX];
    char target_renamed[PATH_MAX];
    char target_renamed2[PATH_MAX];
    char target_link[PATH_MAX];
    char target_link_at[PATH_MAX];
    char target_created_dir_at[PATH_MAX];
    char target_remove_at[PATH_MAX];
    char sibling_dir[PATH_MAX];
    char sibling_file[PATH_MAX];
    char sibling_guard[PATH_MAX];
    char buf[PATH_MAX];
    char read_buf[128];
    struct stat st;
    struct statx stx;
    struct timespec times[2];
    int fd;
    int dirfd;
    int target_dirfd;
    ssize_t n;

    make_path(source_dir, sizeof(source_dir), source_root, "tree");
    make_path(source_file, sizeof(source_file), source_dir, "data.txt");
    make_path(source_renamed, sizeof(source_renamed), source_dir, "renamed.txt");
    make_path(source_link, sizeof(source_link), source_dir, "hard.txt");
    make_path(source_abs_symlink, sizeof(source_abs_symlink), source_dir, "abs.lnk");
    make_path(source_rel_symlink, sizeof(source_rel_symlink), source_dir, "rel.lnk");
    make_path(source_abs_symlink_at, sizeof(source_abs_symlink_at), source_dir, "abs-at.lnk");
    make_path(source_rel_symlink_at, sizeof(source_rel_symlink_at), source_dir, "rel-at.lnk");
    make_path(source_missing, sizeof(source_missing), source_dir, "missing.txt");
    make_path(source_guard, sizeof(source_guard), source_root, "guard.txt");
    make_path(source_created_dir_at, sizeof(source_created_dir_at), source_dir, "made-at");
    make_path(source_remove_at, sizeof(source_remove_at), source_dir, "remove-at.txt");

    make_path(target_dir, sizeof(target_dir), target_root, "tree");
    make_path(target_file, sizeof(target_file), target_dir, "data.txt");
    make_path(target_renamed, sizeof(target_renamed), target_dir, "renamed.txt");
    make_path(target_renamed2, sizeof(target_renamed2), target_dir, "renamed2.txt");
    make_path(target_link, sizeof(target_link), target_dir, "hard.txt");
    make_path(target_link_at, sizeof(target_link_at), target_dir, "hard-at.txt");
    make_path(target_created_dir_at, sizeof(target_created_dir_at), target_dir, "made-at");
    make_path(target_remove_at, sizeof(target_remove_at), target_dir, "remove-at.txt");

    make_path(sibling_dir, sizeof(sibling_dir), sibling_root, "tree");
    make_path(sibling_file, sizeof(sibling_file), sibling_dir, "data.txt");
    make_path(sibling_guard, sizeof(sibling_guard), sibling_root, "guard.txt");

    if (mkdir(target_dir, 0755) != 0 && errno != EEXIST) {
        die("mkdir target tree");
    }
    if (mkdir(sibling_dir, 0755) != 0 && errno != EEXIST) {
        die("mkdir sibling tree");
    }

    write_file_exact(target_file, payload);
    write_file_exact(sibling_file, "sibling payload");
    write_file_exact(sibling_guard, "sibling guard");
    write_file_exact(target_remove_at, "remove me");

    fd = open(source_file, O_RDONLY);
    if (fd < 0) {
        die("open rewritten file");
    }
    memset(read_buf, 0, sizeof(read_buf));
    n = read(fd, read_buf, sizeof(read_buf));
    if (n != (ssize_t)(sizeof(payload) - 1)) {
        die("read rewritten file");
    }
    expect_bytes("open rewritten file", read_buf, payload, sizeof(payload) - 1);
    if (close(fd) != 0) {
        die("close rewritten file");
    }

    fd = creat(source_file, 0644);
    if (fd < 0) {
        die("creat rewritten file");
    }
    if (write(fd, payload, sizeof(payload) - 1) != (ssize_t)(sizeof(payload) - 1)) {
        die("write rewritten file");
    }
    if (close(fd) != 0) {
        die("close creat rewritten file");
    }

    dirfd = open(source_dir, O_RDONLY | O_DIRECTORY);
    if (dirfd < 0) {
        die("open rewritten dir");
    }
    target_dirfd = open(target_dir, O_RDONLY | O_DIRECTORY);
    if (target_dirfd < 0) {
        die("open target dir");
    }
    fd = openat(dirfd, "data.txt", O_RDONLY);
    if (fd < 0) {
        die("openat rewritten file");
    }
    if (close(fd) != 0) {
        die("close openat rewritten file");
    }

    if (stat(source_file, &st) != 0) {
        die("stat rewritten file");
    }
    expect_size("stat rewritten size", st.st_size, sizeof(payload) - 1);

    if (lstat(source_file, &st) != 0) {
        die("lstat rewritten file");
    }
    expect_size("lstat rewritten size", st.st_size, sizeof(payload) - 1);

    if (rename(source_file, source_renamed) != 0) {
        die("rename rewritten file");
    }
    if (stat(target_renamed, &st) != 0) {
        die("stat target renamed");
    }
    if (renameat(dirfd, "renamed.txt", dirfd, "renamed2.txt") != 0) {
        die("renameat rewritten file");
    }
    if (stat(target_renamed2, &st) != 0) {
        die("stat target renamed2");
    }
    if (renameat2(dirfd, "renamed2.txt", dirfd, "renamed.txt", 0) != 0) {
        die("renameat2 rewritten file");
    }
    if (stat(target_renamed, &st) != 0) {
        die("stat target renameat2");
    }
    if (link(source_renamed, source_link) != 0) {
        die("link rewritten file");
    }
    if (stat(target_link, &st) != 0) {
        die("stat target hardlink");
    }
    if (st.st_nlink < 2) {
        fail_message("rewrite hardlink count mismatch");
    }
    if (linkat(dirfd, "renamed.txt", dirfd, "hard-at.txt", 0) != 0) {
        die("linkat rewritten file");
    }
    if (stat(target_link_at, &st) != 0) {
        die("stat target hardlink at");
    }
    if (st.st_nlink < 3) {
        fail_message("rewrite linkat count mismatch");
    }
    if (mkdirat(dirfd, "made-at", 0750) != 0) {
        die("mkdirat rewritten dir");
    }
    if (stat(target_created_dir_at, &st) != 0) {
        die("stat target mkdirat");
    }
    if (!S_ISDIR(st.st_mode)) {
        fail_message("mkdirat target was not a directory");
    }
    if (unlinkat(dirfd, "remove-at.txt", 0) != 0) {
        die("unlinkat rewritten file");
    }
    errno = 0;
    if (stat(target_remove_at, &st) != -1 || errno != ENOENT) {
        fail_message("unlinkat did not remove rewritten target");
    }

    if (symlink(source_renamed, source_abs_symlink) != 0) {
        die("symlink absolute rewritten");
    }
    memset(buf, 0, sizeof(buf));
    n = readlink(source_abs_symlink, buf, sizeof(buf) - 1);
    if (n < 0) {
        die("readlink absolute rewritten");
    }
    buf[n] = '\0';
    if (strcmp(buf, target_renamed) != 0) {
        fail_message("absolute symlink payload was not rewritten");
    }

    if (symlink("../data.txt", source_rel_symlink) != 0) {
        die("symlink relative payload");
    }
    memset(buf, 0, sizeof(buf));
    n = readlink(source_rel_symlink, buf, sizeof(buf) - 1);
    if (n < 0) {
        die("readlink relative payload");
    }
    buf[n] = '\0';
    if (strcmp(buf, "../data.txt") != 0) {
        fail_message("relative symlink payload should be preserved");
    }
    if (symlinkat(source_renamed, dirfd, "abs-at.lnk") != 0) {
        die("symlinkat absolute rewritten");
    }
    memset(buf, 0, sizeof(buf));
    n = readlink(source_abs_symlink_at, buf, sizeof(buf) - 1);
    if (n < 0) {
        die("readlink absolute symlinkat");
    }
    buf[n] = '\0';
    if (strcmp(buf, target_renamed) != 0) {
        fail_message("absolute symlinkat payload was not rewritten");
    }
    memset(buf, 0, sizeof(buf));
    n = readlinkat(dirfd, "abs-at.lnk", buf, sizeof(buf) - 1);
    if (n < 0) {
        die("readlinkat absolute symlink");
    }
    buf[n] = '\0';
    if (strcmp(buf, target_renamed) != 0) {
        fail_message("readlinkat payload was not rewritten");
    }
    if (symlinkat("../data.txt", dirfd, "rel-at.lnk") != 0) {
        die("symlinkat relative payload");
    }
    memset(buf, 0, sizeof(buf));
    n = readlink(source_rel_symlink_at, buf, sizeof(buf) - 1);
    if (n < 0) {
        die("readlink relative symlinkat");
    }
    buf[n] = '\0';
    if (strcmp(buf, "../data.txt") != 0) {
        fail_message("relative symlinkat payload should be preserved");
    }
    times[0].tv_sec = 1700000000;
    times[0].tv_nsec = 123456789;
    times[1].tv_sec = 1700000001;
    times[1].tv_nsec = 222333444;
    if (utimensat(dirfd, "renamed.txt", times, 0) != 0) {
        die("utimensat rewritten file");
    }
    if (stat(target_renamed, &st) != 0) {
        die("stat target utimensat");
    }
    if (st.st_mtime != times[1].tv_sec) {
        fail_message("utimensat mtime mismatch");
    }
    memset(&stx, 0, sizeof(stx));
    if (statx(dirfd, "renamed.txt", 0, STATX_SIZE | STATX_MTIME, &stx) != 0) {
        die("statx rewritten file");
    }
    if ((stx.stx_mask & STATX_SIZE) == 0 || stx.stx_size != sizeof(payload) - 1) {
        fail_message("statx size mismatch");
    }

    fd = open(sibling_file, O_RDONLY);
    if (fd < 0) {
        die("open sibling file");
    }
    memset(read_buf, 0, sizeof(read_buf));
    n = read(fd, read_buf, sizeof(read_buf));
    if (n != (ssize_t)strlen("sibling payload")) {
        die("read sibling file");
    }
    expect_bytes("sibling file payload", read_buf, "sibling payload", strlen("sibling payload"));
    if (close(fd) != 0) {
        die("close sibling file");
    }

    errno = 0;
    if (open(source_guard, O_RDONLY) != -1) {
        fail_message("rewritten missing open unexpectedly succeeded");
    }
    expect_errno_int("rewritten missing open", errno, ENOENT);

    errno = 0;
    if (stat(source_guard, &st) != -1) {
        fail_message("rewritten missing stat unexpectedly succeeded");
    }
    expect_errno_int("rewritten missing stat", errno, ENOENT);

    errno = 0;
    if (lstat(source_guard, &st) != -1) {
        fail_message("rewritten missing lstat unexpectedly succeeded");
    }
    expect_errno_int("rewritten missing lstat", errno, ENOENT);

    errno = 0;
    if (readlink(source_missing, buf, sizeof(buf)) != -1) {
        fail_message("rewritten missing readlink unexpectedly succeeded");
    }
    expect_errno_int("rewritten missing readlink", errno, ENOENT);

    errno = 0;
    if (readlinkat(dirfd, "missing.txt", buf, sizeof(buf)) != -1) {
        fail_message("rewritten missing readlinkat unexpectedly succeeded");
    }
    expect_errno_int("rewritten missing readlinkat", errno, ENOENT);

    errno = 0;
    if (statx(dirfd, "missing.txt", 0, STATX_SIZE, &stx) != -1) {
        fail_message("rewritten missing statx unexpectedly succeeded");
    }
    expect_errno_int("rewritten missing statx", errno, ENOENT);

    if (close(target_dirfd) != 0) {
        die("close target dir");
    }
    if (close(dirfd) != 0) {
        die("close rewritten dir");
    }
}

int main(int argc, char **argv) {
    fixture_argv0 = argv[0];
    if (argc < 2) {
        fprintf(stderr,
                "usage: %s <basic|metadata|missing|passthrough|failopen|rewrite|xattr|fd-semantics|fd-exec-check> [args]\n",
                argv[0]);
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
    if (strcmp(argv[1], "passthrough") == 0) {
        if (argc != 4) {
            fprintf(stderr, "usage: %s passthrough <source-root> <target-root>\n", argv[0]);
            return 2;
        }
        run_passthrough(argv[2], argv[3]);
        return 0;
    }
    if (strcmp(argv[1], "failopen") == 0) {
        run_failopen();
        return 0;
    }
    if (strcmp(argv[1], "rewrite") == 0) {
        if (argc != 5) {
            fprintf(stderr, "usage: %s rewrite <source-root> <target-root> <sibling-root>\n",
                    argv[0]);
            return 2;
        }
        run_rewrite(argv[2], argv[3], argv[4]);
        return 0;
    }
    if (strcmp(argv[1], "fd-semantics") == 0) {
        if (argc != 4) {
            fprintf(stderr, "usage: %s fd-semantics <source-root> <target-root>\n", argv[0]);
            return 2;
        }
        run_fd_semantics(argv[2], argv[3]);
        return 0;
    }
    if (strcmp(argv[1], "fd-exec-check") == 0) {
        if (argc != 4) {
            fprintf(stderr, "usage: %s fd-exec-check <keep-fd> <cloexec-fd>\n", argv[0]);
            return 2;
        }
        run_fd_exec_check(atoi(argv[2]), atoi(argv[3]));
        return 0;
    }

    fprintf(stderr, "unknown scenario: %s\n", argv[1]);
    return 2;
}

#endif
