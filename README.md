# ld_preload
Tinkering with LD_PRELOAD based file system interceptors in Go

There are few examples of using LD_PRELOAD with Go, but as of 09/24, they are trivial examples.
There is, however, a C library (https://github.com/sholtrop/ldpfuse/) which implements a FUSE-like file system in C.

 ### Preliminary Findings

There are two attempts in this repo's history to build a system-call logging proxy.  Both used exported Go functions with C types as parameters.  The first attempt relied on `"golang.org/x/sys/unix"` functions to fulfill the proxied call.  In testing, this worked for trivial examples, but produced deadlocks in some cases.  I ran `strace` against `ls` and confirmed it was waiting on a futex, rather than being in a loop.  Interstingly, I determined that if I didn't override `access()`, the issue went away, yet `access()` did appear to succeed in other applications.

This brings me to the second attempt.  I suspected the issue stemmed either from how Go exposes the DLL endpoints or from its underlying implementation of the Unix system calls. I tried swapping out the `"golang.org/x/sys/unix"` functions with C functions implementing `dlsym(RTLD_NEXT, "stdfunc")`.  To my chagrin, I get the same deadlock.

### The pains of cgo

 - It's very easy to get an error such as `error: conflicting types for ‘fstat’;`.  For me, a non-C expert, this happens so much I can't really nail down what causes it.  It's is easily avoided by avoiding C standard imports.
 - Go structs cannot be used from C.  Notably, Go structs cannot be used as parameters in Go functions marked for `export`.  At best, you can use an `unsafe.Pointer` in a function defintion and then cast to a byte-equivalent Go struct.
 - Some C-analogous Go types can be used in Go functions marked for `export`.  In practice, however, I've found that some type substitutions produce erratic results, such as `string` vs `*C.char`.  Using C types therefore is preferred.
 - Go functions cannot be cast to / from `unsafe.Pointer`, for memory safety reasons.  Go functions can be exposed to C by marking them as `export` createing accompanying C function definitions marked `extern`.
 - As much as "do it in Go" sounds easier, it doesn't spare you from dealing with C.  There are a ton of `#ifdef`s and `#ifndef`s in `<sys/stat.h>`, so there's some logical chain of header files to import first.  Which ones?  Go can't tell ya.
