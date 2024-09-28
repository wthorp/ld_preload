package main

/*
#include "rtld.h"
*/
import "C"
import (
	"fmt"
	"unsafe"

	"golang.org/x/sys/unix"
)

// There's an issue I don't fully understand which prevents the import
// of <sys/stat.h>.  If imported, there are conflicts with the function
// defintions for chmod, mkdir, mknod, and mknodat.

// The functions __fxstat, __fxstatat, __lxstat, __xstat, and fstat all
// rely on the stat structure defined in <sys/stat.h>.  Unsafe pointers
// could be used, but instead we simply define the struct above.

//export __fxstat
func __fxstat(_ C.int, fd C.int, cstat *C.struct_stat) C.int {
	fmt.Println("In __fxstat")
	if cstat == nil {
		return -1
	}
	ustat := (*unix.Stat_t)(unsafe.Pointer(cstat))
	err := unix.Fstat(int(fd), ustat)
	if err != nil {
		return -1
	}
	return 0
}

//export __fxstatat
func __fxstatat(_ C.int, dirfd C.int, pathname *C.char, cstat *C.struct_stat, flags C.int) C.int {
	fmt.Println("In __fxstatat")
	if cstat == nil || pathname == nil {
		return -1
	}
	ustat := (*unix.Stat_t)(unsafe.Pointer(cstat))
	err := unix.Fstatat(int(dirfd), C.GoString(pathname), ustat, int(flags))
	if err != nil {
		return -1
	}
	return 0
}

//export __lxstat
func __lxstat(_ C.int, pathname *C.char, cstat *C.struct_stat) C.int {
	fmt.Println("In __lxstat")
	if cstat == nil || pathname == nil {
		return -1
	}
	ustat := (*unix.Stat_t)(unsafe.Pointer(cstat))
	err := unix.Lstat(C.GoString(pathname), ustat)
	if err != nil {
		return -1
	}
	return 0
}

//export __xstat
func __xstat(_ C.int, pathname *C.char, cstat *C.struct_stat) C.int {
	fmt.Println("In __xstat")
	if cstat == nil || pathname == nil {
		return -1
	}
	ustat := (*unix.Stat_t)(unsafe.Pointer(cstat))
	err := unix.Stat(C.GoString(pathname), ustat)
	if err != nil {
		return -1
	}
	return 0
}

//export access
func access(pathname *C.char, mode C.int) C.int {
	fmt.Println("In access")
	err := unix.Access(C.GoString(pathname), uint32(mode))
	if err != nil {
		return -1
	}
	return 0
}

//export chmod
func chmod(pathname *C.char, mode C.mode_t) C.int {
	fmt.Println("In chmod")
	err := unix.Chmod(C.GoString(pathname), uint32(mode))
	if err != nil {
		return -1
	}
	return 0
}

//export chown
func chown(pathname *C.char, owner C.uid_t, group C.gid_t) C.int {
	fmt.Println("In chown")
	err := unix.Chown(C.GoString(pathname), int(owner), int(group))
	if err != nil {
		return -1
	}
	return 0
}

//export close
func close(fd C.int) C.int {
	fmt.Println("In close")
	err := unix.Close(int(fd))
	if err != nil {
		return -1
	}
	return 0
}

//export creat
func creat(pathname *C.char, mode C.mode_t) C.int {
	fmt.Println("In creat")
	fd, err := unix.Creat(C.GoString(pathname), uint32(mode))
	if err != nil {
		return -1
	}
	return C.int(fd)
}

//export euidaccess
func euidaccess(pathname *C.char, mode C.int) C.int {
	fmt.Println("In euidaccess")
	err := unix.Access(C.GoString(pathname), uint32(mode))
	if err != nil {
		return -1
	}
	return 0
}

//export faccessat
func faccessat(dirfd C.int, pathname *C.char, mode C.int, flags C.int) C.int {
	fmt.Println("In faccessat")
	err := unix.Faccessat(int(dirfd), C.GoString(pathname), uint32(mode), int(flags))
	if err != nil {
		return -1
	}
	return 0
}

//export fgetxattr
func fgetxattr(fd C.int, name *C.char, value unsafe.Pointer, size C.size_t) C.ssize_t {
	fmt.Println("In fgetxattr")
	buf := make([]byte, size)
	n, err := unix.Fgetxattr(int(fd), C.GoString(name), buf)
	if err != nil {
		return -1
	}
	copy(C.GoBytes(value, C.int(size)), buf[:n])
	return C.ssize_t(n)
}

//export fstat
func fstat(fd C.int, cstat *C.struct_stat) C.int {
	fmt.Println("In fstat")
	ustat := (*unix.Stat_t)(unsafe.Pointer(cstat))
	error := unix.Fstat(int(fd), ustat)
	if error != nil {
		return -1
	}
	return 0
}

//export getxattr
func getxattr(pathname *C.char, name *C.char, value unsafe.Pointer, size C.size_t) C.ssize_t {
	fmt.Println("In getxattr")
	buf := make([]byte, size)
	n, err := unix.Getxattr(C.GoString(pathname), C.GoString(name), buf)
	if err != nil {
		return -1
	}
	copy(C.GoBytes(value, C.int(size)), buf[:n])
	return C.ssize_t(n)
}

//export lgetxattr
func lgetxattr(pathname *C.char, name *C.char, value unsafe.Pointer, size C.size_t) C.ssize_t {
	fmt.Println("In lgetxattr")
	buf := make([]byte, size)
	n, err := unix.Lgetxattr(C.GoString(pathname), C.GoString(name), buf)
	if err != nil {
		return -1
	}
	copy(C.GoBytes(value, C.int(size)), buf[:n])
	return C.ssize_t(n)
}

//export link
func link(oldpath *C.char, newpath *C.char) C.int {
	fmt.Println("In link")
	err := unix.Link(C.GoString(oldpath), C.GoString(newpath))
	if err != nil {
		return -1
	}
	return 0
}

//export lseek
func lseek(fd C.int, offset C.off_t, whence C.int) C.off_t {
	fmt.Println("In lseek")
	newOffset, err := unix.Seek(int(fd), int64(offset), int(whence))
	if err != nil {
		return -1
	}
	return C.off_t(newOffset)
}

//export mkdir
func mkdir(pathname *C.char, mode C.mode_t) C.int {
	fmt.Println("In mkdir")
	err := unix.Mkdir(C.GoString(pathname), uint32(mode))
	if err != nil {
		return -1
	}
	return 0
}

//export mknod
func mknod(pathname *C.char, mode C.mode_t, dev C.dev_t) C.int {
	fmt.Println("In mknod")
	err := unix.Mknod(C.GoString(pathname), uint32(mode), int(dev))
	if err != nil {
		return -1
	}
	return 0
}

//export mknodat
func mknodat(dirfd C.int, pathname *C.char, mode C.mode_t, dev C.dev_t) C.int {
	fmt.Println("In mknodat")
	err := unix.Mknodat(int(dirfd), C.GoString(pathname), uint32(mode), int(dev))
	if err != nil {
		return -1
	}
	return 0
}

//export open
func open(pathname *C.char, flags C.int, mode C.mode_t) C.int {
	fmt.Println("In open")
	fd, err := unix.Open(C.GoString(pathname), int(flags), uint32(mode))
	if err != nil {
		return -1
	}
	return C.int(fd)
}

//export openat
func openat(dirfd C.int, pathname *C.char, flags C.int, mode C.mode_t) C.int {
	fmt.Println("In openat")
	fd, err := unix.Openat(int(dirfd), C.GoString(pathname), int(flags), uint32(mode))
	if err != nil {
		return -1
	}
	return C.int(fd)
}

//export pread
func pread(fd C.int, buf unsafe.Pointer, count C.size_t, offset C.off_t) C.ssize_t {
	fmt.Println("In pread")
	n, err := unix.Pread(int(fd), (*[1 << 30]byte)(buf)[:count], int64(offset))
	if err != nil {
		return -1
	}
	return C.ssize_t(n)
}

//export pwrite
func pwrite(fd C.int, buf unsafe.Pointer, count C.size_t, offset C.off_t) C.ssize_t {
	fmt.Println("In pwrite")
	n, err := unix.Pwrite(int(fd), (*[1 << 30]byte)(buf)[:count], int64(offset))
	if err != nil {
		return -1
	}
	return C.ssize_t(n)
}

//export read
func read(fd C.int, buf unsafe.Pointer, count C.size_t) C.ssize_t {
	fmt.Println("In read")
	n, err := unix.Read(int(fd), (*[1 << 30]byte)(buf)[:count])
	if err != nil {
		return -1
	}
	return C.ssize_t(n)
}

//export readlink
func readlink(pathname *C.char, buf unsafe.Pointer, bufsiz C.size_t) C.ssize_t {
	fmt.Println("In readlink")
	n, err := unix.Readlink(C.GoString(pathname), (*[1 << 30]byte)(buf)[:bufsiz])
	if err != nil {
		return -1
	}
	return C.ssize_t(n)
}

//export rename
func rename(oldpath *C.char, newpath *C.char) C.int {
	fmt.Println("In rename")
	err := unix.Rename(C.GoString(oldpath), C.GoString(newpath))
	if err != nil {
		return -1
	}
	return 0
}

//export rmdir
func rmdir(pathname *C.char) C.int {
	fmt.Println("In rmdir")
	err := unix.Rmdir(C.GoString(pathname))
	if err != nil {
		return -1
	}
	return 0
}

//export symlink
func symlink(target *C.char, linkpath *C.char) C.int {
	fmt.Println("In symlink")
	err := unix.Symlink(C.GoString(target), C.GoString(linkpath))
	if err != nil {
		return -1
	}
	return 0
}

//export truncate
func truncate(pathname *C.char, length C.off_t) C.int {
	fmt.Println("In truncate")
	err := unix.Truncate(C.GoString(pathname), int64(length))
	if err != nil {
		return -1
	}
	return 0
}

//export unlink
func unlink(pathname *C.char) C.int {
	fmt.Println("In unlink")
	err := unix.Unlink(C.GoString(pathname))
	if err != nil {
		return -1
	}
	return 0
}

//export write
func write(fd C.int, buf unsafe.Pointer, count C.size_t) C.ssize_t {
	fmt.Println("In write")
	n, err := unix.Write(int(fd), (*[1 << 30]byte)(buf)[:count])
	if err != nil {
		return -1
	}
	return C.ssize_t(n)
}
