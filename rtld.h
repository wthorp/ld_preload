#ifndef RTLD_H
#define RTLD_H

#ifndef __linux__
#error "rtld.h is only supported on Linux"
#endif

#define _GNU_SOURCE

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <sys/statvfs.h>
#include <linux/openat2.h>
#include <unistd.h>

int orig___fxstat(int ver, int fd, struct stat *cstat);
int orig___fxstatat(int ver, int dirfd, const char *pathname, struct stat *cstat, int flags);
int orig___lxstat(int ver, const char *pathname, struct stat *cstat);
int orig___xstat(int ver, const char *pathname, struct stat *cstat);
int orig_chdir(const char *path);
int orig_close(int fd);
int orig_execve(const char *pathname, char *const argv[], char *const envp[]);
int orig_creat(const char *pathname, mode_t mode);
int orig_fstat(int fd, struct stat *cstat);
int orig_newfstatat(int dirfd, const char *pathname, struct stat *statbuf, int flags);
int orig_faccessat(int dirfd, const char *pathname, int mode, int flags);
int orig_fchmodat(int dirfd, const char *pathname, mode_t mode, int flags);
int orig_link(const char *oldpath, const char *newpath);
int orig_linkat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags);
off_t orig_lseek(int fd, off_t offset, int whence);
int orig_mkdir(const char *pathname, mode_t mode);
int orig_mkdirat(int dirfd, const char *pathname, mode_t mode);
int orig_open(const char *pathname, int flags, mode_t mode);
int orig_openat(int dirfd, const char *pathname, int flags, mode_t mode);
int orig_openat2(int dirfd, const char *pathname, const struct open_how *how, size_t size);
ssize_t orig_pread(int fd, void *buf, size_t count, off_t offset);
ssize_t orig_pwrite(int fd, const void *buf, size_t count, off_t offset);
ssize_t orig_read(int fd, void *buf, size_t count);
ssize_t orig_readlink(const char *pathname, void *buf, size_t bufsiz);
ssize_t orig_readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz);
int orig_rename(const char *oldpath, const char *newpath);
int orig_renameat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath);
int orig_renameat2(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, unsigned int flags);
int orig_rmdir(const char *pathname);
int orig_lstat(const char *pathname, struct stat *cstat);
int orig_statx(int dirfd, const char *pathname, int flags, unsigned int mask, struct statx *statxbuf);
int orig_stat(const char *pathname, struct stat *cstat);
int orig_statfs(const char *path, struct statvfs *buf);
int orig_symlink(const char *target, const char *linkpath);
int orig_symlinkat(const char *target, int newdirfd, const char *linkpath);
int orig_truncate(const char *pathname, off_t length);
int orig_unlink(const char *pathname);
int orig_unlinkat(int dirfd, const char *pathname, int flags);
int orig_utimensat(int dirfd, const char *pathname, const struct timespec times[2], int flags);
char *orig_getcwd(char *buf, size_t size);
ssize_t orig_write(int fd, const void *buf, size_t count);

#endif
