#define RTLD_NEXT ((void *) -1L)
#include "rtld.h"
#include <dlfcn.h>

int orig___fxstat(int ver, int fd, struct stat *cstat) {
    typedef int (*orig_fstat_t)(int, int, struct stat*);
    orig_fstat_t orig_fstat = (orig_fstat_t) dlsym(RTLD_NEXT, "__fxstat");
    if (!orig_fstat) return -1;
    return orig_fstat(ver, fd, cstat);
}

int orig___fxstatat(int ver, int dirfd, const char *pathname, struct stat *cstat, int flags) {
    typedef int (*orig_fxstatat_t)(int, int, const char*, struct stat*, int);
    orig_fxstatat_t orig_fxstatat = (orig_fxstatat_t) dlsym(RTLD_NEXT, "__fxstatat");
    if (!orig_fxstatat) return -1;
    return orig_fxstatat(ver, dirfd, pathname, cstat, flags);
}

int orig___lxstat(int ver, const char *pathname, struct stat *cstat) {
    typedef int (*orig_lxstat_t)(int, const char*, struct stat*);
    orig_lxstat_t orig_lxstat = (orig_lxstat_t) dlsym(RTLD_NEXT, "__lxstat");
    if (!orig_lxstat) return -1;
    return orig_lxstat(ver, pathname, cstat);
}

int orig___xstat(int ver, const char *pathname, struct stat *cstat) {
    typedef int (*orig_xstat_t)(int, const char*, struct stat*);
    orig_xstat_t orig_xstat = (orig_xstat_t) dlsym(RTLD_NEXT, "__xstat");
    if (!orig_xstat) return -1;
    return orig_xstat(ver, pathname, cstat);
}

int orig_access(const char *pathname, int mode) {
    typedef int (*orig_access_t)(const char*, int);
    orig_access_t orig_access = (orig_access_t) dlsym(RTLD_NEXT, "access");
    if (!orig_access) return -1;
    return orig_access(pathname, mode);
}

int orig_chmod(const char *pathname, mode_t mode) {
    typedef int (*orig_chmod_t)(const char*, mode_t);
    orig_chmod_t orig_chmod = (orig_chmod_t) dlsym(RTLD_NEXT, "chmod");
    if (!orig_chmod) return -1;
    return orig_chmod(pathname, mode);
}

int orig_chown(const char *pathname, uid_t owner, gid_t group) {
    typedef int (*orig_chown_t)(const char*, uid_t, gid_t);
    orig_chown_t orig_chown = (orig_chown_t) dlsym(RTLD_NEXT, "chown");
    if (!orig_chown) return -1;
    return orig_chown(pathname, owner, group);
}

int orig_close(int fd) {
    typedef int (*orig_close_t)(int);
    orig_close_t orig_close = (orig_close_t) dlsym(RTLD_NEXT, "close");
    if (!orig_close) return -1;
    return orig_close(fd);
}

int orig_creat(const char *pathname, mode_t mode) {
    typedef int (*orig_creat_t)(const char*, mode_t);
    orig_creat_t orig_creat = (orig_creat_t) dlsym(RTLD_NEXT, "creat");
    if (!orig_creat) return -1;
    return orig_creat(pathname, mode);
}

int orig_euidaccess(const char *pathname, int mode) {
    typedef int (*orig_euidaccess_t)(const char*, int);
    orig_euidaccess_t orig_euidaccess = (orig_euidaccess_t) dlsym(RTLD_NEXT, "euidaccess");
    if (!orig_euidaccess) return -1;
    return orig_euidaccess(pathname, mode);
}

int orig_faccessat(int dirfd, const char *pathname, int mode, int flags) {
    typedef int (*orig_faccessat_t)(int, const char*, int, int);
    orig_faccessat_t orig_faccessat = (orig_faccessat_t) dlsym(RTLD_NEXT, "faccessat");
    if (!orig_faccessat) return -1;
    return orig_faccessat(dirfd, pathname, mode, flags);
}

ssize_t orig_fgetxattr(int fd, const char *name, void *value, size_t size) {
    typedef ssize_t (*orig_fgetxattr_t)(int, const char*, void*, size_t);
    orig_fgetxattr_t orig_fgetxattr = (orig_fgetxattr_t) dlsym(RTLD_NEXT, "fgetxattr");
    if (!orig_fgetxattr) return -1;
    return orig_fgetxattr(fd, name, value, size);
}

int orig_fstat(int fd, struct stat *cstat) {
    typedef int (*orig_fstat_t)(int, struct stat*);
    orig_fstat_t orig_fstat = (orig_fstat_t) dlsym(RTLD_NEXT, "fstat");
    if (!orig_fstat) return -1;
    return orig_fstat(fd, cstat);
}

ssize_t orig_getxattr(const char *pathname, const char *name, void *value, size_t size) {
    typedef ssize_t (*orig_getxattr_t)(const char*, const char*, void*, size_t);
    orig_getxattr_t orig_getxattr = (orig_getxattr_t) dlsym(RTLD_NEXT, "getxattr");
    if (!orig_getxattr) return -1;
    return orig_getxattr(pathname, name, value, size);
}

ssize_t orig_lgetxattr(const char *pathname, const char *name, void *value, size_t size) {
    typedef ssize_t (*orig_lgetxattr_t)(const char*, const char*, void*, size_t);
    orig_lgetxattr_t orig_lgetxattr = (orig_lgetxattr_t) dlsym(RTLD_NEXT, "lgetxattr");
    if (!orig_lgetxattr) return -1;
    return orig_lgetxattr(pathname, name, value, size);
}

int orig_link(const char *oldpath, const char *newpath) {
    typedef int (*orig_link_t)(const char*, const char*);
    orig_link_t orig_link = (orig_link_t) dlsym(RTLD_NEXT, "link");
    if (!orig_link) return -1;
    return orig_link(oldpath, newpath);
}

off_t orig_lseek(int fd, off_t offset, int whence) {
    typedef off_t (*orig_lseek_t)(int, off_t, int);
    orig_lseek_t orig_lseek = (orig_lseek_t) dlsym(RTLD_NEXT, "lseek");
    if (!orig_lseek) return -1;
    return orig_lseek(fd, offset, whence);
}

int orig_mkdir(const char *pathname, mode_t mode) {
    typedef int (*orig_mkdir_t)(const char*, mode_t);
    orig_mkdir_t orig_mkdir = (orig_mkdir_t) dlsym(RTLD_NEXT, "mkdir");
    if (!orig_mkdir) return -1;
    return orig_mkdir(pathname, mode);
}

int orig_mknod(const char *pathname, mode_t mode, dev_t dev) {
    typedef int (*orig_mknod_t)(const char*, mode_t, dev_t);
    orig_mknod_t orig_mknod = (orig_mknod_t) dlsym(RTLD_NEXT, "mknod");
    if (!orig_mknod) return -1;
    return orig_mknod(pathname, mode, dev);
}

int orig_mknodat(int dirfd, const char *pathname, mode_t mode, dev_t dev) {
    typedef int (*orig_mknodat_t)(int, const char*, mode_t, dev_t);
    orig_mknodat_t orig_mknodat = (orig_mknodat_t) dlsym(RTLD_NEXT, "mknodat");
    if (!orig_mknodat) return -1;
    return orig_mknodat(dirfd, pathname, mode, dev);
}

int orig_open(const char *pathname, int flags, mode_t mode) {
    typedef int (*orig_open_t)(const char*, int, mode_t);
    orig_open_t orig_open = (orig_open_t) dlsym(RTLD_NEXT, "open");
    if (!orig_open) return -1;
    return orig_open(pathname, flags, mode);
}

int orig_openat(int dirfd, const char *pathname, int flags, mode_t mode) {
    typedef int (*orig_openat_t)(int, const char*, int, mode_t);
    orig_openat_t orig_openat = (orig_openat_t) dlsym(RTLD_NEXT, "openat");
    if (!orig_openat) return -1;
    return orig_openat(dirfd, pathname, flags, mode);
}

ssize_t orig_pread(int fd, void *buf, size_t count, off_t offset) {
    typedef ssize_t (*orig_pread_t)(int, void*, size_t, off_t);
    orig_pread_t orig_pread = (orig_pread_t) dlsym(RTLD_NEXT, "pread");
    if (!orig_pread) return -1;
    return orig_pread(fd, buf, count, offset);
}

ssize_t orig_pwrite(int fd, const void *buf, size_t count, off_t offset) {
    typedef ssize_t (*orig_pwrite_t)(int, const void*, size_t, off_t);
    orig_pwrite_t orig_pwrite = (orig_pwrite_t) dlsym(RTLD_NEXT, "pwrite");
    if (!orig_pwrite) return -1;
    return orig_pwrite(fd, buf, count, offset);
}

ssize_t orig_read(int fd, void *buf, size_t count) {
    typedef ssize_t (*orig_read_t)(int, void*, size_t);
    orig_read_t orig_read = (orig_read_t) dlsym(RTLD_NEXT, "read");
    if (!orig_read) return -1;
    return orig_read(fd, buf, count);
}

ssize_t orig_readlink(const char *pathname, void *buf, size_t bufsiz) {
    typedef ssize_t (*orig_readlink_t)(const char*, void*, size_t);
    orig_readlink_t orig_readlink = (orig_readlink_t) dlsym(RTLD_NEXT, "readlink");
    if (!orig_readlink) return -1;
    return orig_readlink(pathname, buf, bufsiz);
}

int orig_rename(const char *oldpath, const char *newpath) {
    typedef int (*orig_rename_t)(const char*, const char*);
    orig_rename_t orig_rename = (orig_rename_t) dlsym(RTLD_NEXT, "rename");
    if (!orig_rename) return -1;
    return orig_rename(oldpath, newpath);
}

int orig_rmdir(const char *pathname) {
    typedef int (*orig_rmdir_t)(const char*);
    orig_rmdir_t orig_rmdir = (orig_rmdir_t) dlsym(RTLD_NEXT, "rmdir");
    if (!orig_rmdir) return -1;
    return orig_rmdir(pathname);
}

int orig_symlink(const char *target, const char *linkpath) {
    typedef int (*orig_symlink_t)(const char*, const char*);
    orig_symlink_t orig_symlink = (orig_symlink_t) dlsym(RTLD_NEXT, "symlink");
    if (!orig_symlink) return -1;
    return orig_symlink(target, linkpath);
}

int orig_truncate(const char *pathname, off_t length) {
    typedef int (*orig_truncate_t)(const char*, off_t);
    orig_truncate_t orig_truncate = (orig_truncate_t) dlsym(RTLD_NEXT, "truncate");
    if (!orig_truncate) return -1;
    return orig_truncate(pathname, length);
}

int orig_unlink(const char *pathname) {
    typedef int (*orig_unlink_t)(const char*);
    orig_unlink_t orig_unlink = (orig_unlink_t) dlsym(RTLD_NEXT, "unlink");
    if (!orig_unlink) return -1;
    return orig_unlink(pathname);
}

ssize_t orig_write(int fd, const void *buf, size_t count) {
    typedef ssize_t (*orig_write_t)(int, const void*, size_t);
    orig_write_t orig_write = (orig_write_t) dlsym(RTLD_NEXT, "write");
    if (!orig_write) return -1;
    return orig_write(fd, buf, count);
}
