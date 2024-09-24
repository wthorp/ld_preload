package main

/*
#include <sys/stat.h>
*/
import "C"
import (
	"fmt"
	"unsafe"

	"golang.org/x/sys/unix"
)

//export fstat
func fstat(fd C.int, cstat *C.struct_stat) C.int {
	fmt.Printf("fstat(%d, %p)\n", fd, cstat)
	if cstat == nil {
		return -1
	}
	ustat := (*unix.Stat_t)(unsafe.Pointer(cstat))
	error := unix.Fstat(int(fd), ustat)
	if error != nil {
		return -1
	}
	return 0
}

func main() {
	shell()
}
