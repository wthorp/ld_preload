# ld_preload
Tinkering with LD_PRELOAD based file system interceptors in Go

There are few examples of using LD_PRELOAD with Go, but as of 09/24, they are trivial examples.
There is, however, a C library (https://github.com/sholtrop/ldpfuse/) which implements a FUSE-like file system in C.

### Possible methodologies

- Leverage the ldpfuse C library.  Expose Go functions to C.  Create a small C wrapper to pass these functions to `ldp_fuse_init`.

- Create largely native Go approach, which minimized C interop.


### Constraints With C-Compatible Exports In Go

 - Go structs cannot be used from C.  Notably, Go structs cannot be used as parameters in Go functions marked for `export`.  In this sense, some C code is needed for a FUSE-like solution, due to C structs such as `stat`.
 - It's very easy to improperly define a function and get an error such as `cgo-gcc-export-header-prolog:55:28: error: conflicting types for ‘fstat’;`.  This happens when 
 the function defintion isn't identical to the function being overwritten.
 - Some C-analogous Go types can be used in Go functions marked for `export`.  In practice, however, I've found that some type substitutions produce erratic results, such as `string` vs `*C.char`.  Using C types therefore is preferred.
 - While C code requires `dlsym(RTLD_NEXT, "stdfunc")` to call functions overridden by the linker, preliminary testing implies Go `syscall` methods continue to work as expected.
 - Go functions cannot be cast to / from `unsafe.Pointer`, for memory safety reasons.  Go functions can be exposed to C by marking them as `export` createing accompanying C function definitions marked `extern`.


### Example Calling Go Functions From C

```
package main

/*
#include <stdlib.h>
extern void go_callback(int);
static void call_callback(int val) {
    go_callback(val);  // Call the callback with the given value
}
*/
import "C"
import "fmt"

//export go_callback
func go_callback(val C.int) {
	fmt.Printf("Go callback called with value: %d\n", val)
}

func main() {
	C.call_callback(42)
}
```