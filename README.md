# ld_preload tinkering
Tinkering with proxying file IO syscalls on linux via an LD_PRELOAD shared object

This project is archived.  However, if you have any success with the techniques here, please let me know!

## Background

The project focuses on exploring the interception of file I/O system calls on Linux by using an LD_PRELOAD shared object. 
This technique allows for the user-space handling of system calls, such as open, read, and write.  This technique can 
double performance for FUSE-like filesystems, by eliminating OS overhead and the security measures required for transferring 
data between user space and kernel space.

There are few examples of using `LD_PRELOAD` with Go, but as of 09/24, they are trivial examples.
[ldpfuse](https://github.com/sholtrop/ldpfuse/) is an open source C library for implementing a FUSE-like file system in C.
[cunoFS](https://cuno.io/) is a commerical example of this technique.

## Preliminary Findings

There are two attempts in this repo's history to build a system-call logging proxy.  Both used exported Go functions with C types as parameters.  The first attempt relied on `"golang.org/x/sys/unix"` functions to fulfill the proxied call.  In testing, this worked for trivial examples, but produced deadlocks in some cases.  I ran `strace` against `ls` and confirmed it was waiting on a futex, rather than being in a loop.  Interstingly, I determined that if I didn't override `access()`, the issue went away, yet `access()` did appear to succeed in other applications.

This brings me to the second attempt.  I suspected the issue stemmed either from how Go exposes the DLL endpoints or from its underlying implementation of the Unix system calls. I tried swapping out the `"golang.org/x/sys/unix"` functions with C functions implementing `dlsym(RTLD_NEXT, "stdfunc")`.  To my chagrin, I get the same deadlock.

## The pains of cgo

 - It's very easy to get an error such as `error: conflicting types for ‘fstat’;`.  I'm sure there a logic to it, but I can't fully nail down when cgo is comfortable overriding functions and when it isn't.  It is, however, easily avoided by avoiding C standard imports entirely.
 - Go structs cannot be used from C.  Notably, Go structs cannot be used as parameters in Go functions marked for `export`.  At best, you can use an `unsafe.Pointer` in a function defintion and then cast to a byte-equivalent Go struct.
 - Some C-analogous Go types can be used in Go functions marked for `export`.  In practice, however, I've found that some type substitutions produce erratic results, such as `string` vs `*C.char`.  Using C types therefore is preferred.
 - Go functions cannot be cast to / from `unsafe.Pointer`, for memory safety reasons.  Go functions can be exposed to C by marking them as `export` createing accompanying C function definitions marked `extern`.
 - As much as "do it in Go" sounds easier, it doesn't spare you from dealing with C.  There are a ton of `#ifdef`s and `#ifndef`s in `<sys/stat.h>`, so there's some logical chain of header files to import first.  Which ones?  Go can't tell ya.
