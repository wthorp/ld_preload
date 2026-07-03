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
#include <unistd.h>

int orig___fxstat(int ver, int fd, struct stat *cstat);
int orig___fxstatat(int ver, int dirfd, const char *pathname, struct stat *cstat, int flags);
int orig___lxstat(int ver, const char *pathname, struct stat *cstat);
int orig___xstat(int ver, const char *pathname, struct stat *cstat);
int orig_access(const char *pathname, int mode);
int orig_chmod(const char *pathname, mode_t mode);
int orig_chown(const char *pathname, uid_t owner, gid_t group);
int orig_close(int fd);
int orig_creat(const char *pathname, mode_t mode);
int orig_euidaccess(const char *pathname, int mode);
int orig_faccessat(int dirfd, const char *pathname, int mode, int flags);
ssize_t orig_fgetxattr(int fd, const char *name, void *value, size_t size);
int orig_fstat(int fd, struct stat *cstat);
ssize_t orig_getxattr(const char *pathname, const char *name, void *value, size_t size);
ssize_t orig_lgetxattr(const char *pathname, const char *name, void *value, size_t size);
int orig_link(const char *oldpath, const char *newpath);
off_t orig_lseek(int fd, off_t offset, int whence);
int orig_mkdir(const char *pathname, mode_t mode);
int orig_mknod(const char *pathname, mode_t mode, dev_t dev);
int orig_mknodat(int dirfd, const char *pathname, mode_t mode, dev_t dev);
int orig_open(const char *pathname, int flags, mode_t mode);
int orig_openat(int dirfd, const char *pathname, int flags, mode_t mode);
ssize_t orig_pread(int fd, void *buf, size_t count, off_t offset);
ssize_t orig_pwrite(int fd, const void *buf, size_t count, off_t offset);
ssize_t orig_read(int fd, void *buf, size_t count);
ssize_t orig_readlink(const char *pathname, void *buf, size_t bufsiz);
int orig_rename(const char *oldpath, const char *newpath);
int orig_rmdir(const char *pathname);
int orig_symlink(const char *target, const char *linkpath);
int orig_truncate(const char *pathname, off_t length);
int orig_unlink(const char *pathname);
ssize_t orig_write(int fd, const void *buf, size_t count);

#endif
