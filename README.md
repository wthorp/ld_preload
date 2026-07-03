# ld_preload

Prototype Linux `LD_PRELOAD` interposer for file-oriented libc calls, with Go used around the edges rather than in the hot interception path.

This repo is aimed at developers who want to understand or experiment with the design. It is not presented as a production-ready filesystem layer.

## What this is

This project explores a FUSE-like idea: intercept common file I/O entry points in a shared object and decide in user space how those calls should behave.

The current shape is intentionally conservative:

- the exported interposer symbols live in C
- the interposer resolves the real libc entry points with `dlsym(RTLD_NEXT, ...)`
- the Go side is kept out of the direct libc hook boundary

That design exists because the more direct "export Go functions as the overridden libc symbols" approach was unstable in practice.

## Status

Treat this as a working prototype with caveats.

What is true today:

- the library builds as a Linux `c-shared` interposer
- the repo has a stronger developer workflow than it used to
- the interposer path is exercised against real Linux userlands

What is not true today:

- this is not a finished virtual filesystem implementation
- this does not claim broad libc or distro compatibility
- this does not try to promise production safety under arbitrary process startup paths

## Why the design looks like this

Earlier attempts in this repo tried to override libc symbols with exported Go functions and then fulfill the work with Go syscall wrappers. That looked appealing on paper and worked for trivial cases, but it produced deadlocks in real processes. In particular, routing intercepted libc calls through the Go runtime was too risky in loader-sensitive paths.

The current direction is a reaction to that experience:

- keep exact libc-facing signatures in C
- keep `open` and `openat` handling variadic at the C boundary
- use real Linux headers instead of hand-rolled ABI definitions where possible
- treat Go as an implementation language behind the boundary, not the boundary itself

If you are here to build on the idea, this is the main architectural decision to understand first.

## Minimal usage

This repo is Linux-specific where the interposer matters.

At a high level:

1. Build the shared object.
2. Preload it into a target process with `LD_PRELOAD`.
3. Observe behavior on ordinary libc consumers before attempting anything more ambitious.

The included Go executable is only a convenience wrapper for launching a shell with the preload set. It is not the core of the design.

## The pains of cgo

These are still the most relevant lessons from working on this:

- It is very easy to get errors like `conflicting types for 'fstat'` when the cgo-generated declarations do not match the C ABI exactly.
- Go structs cannot be used directly from C exports. In practice that means using C types at the boundary and converting with `unsafe.Pointer` only when you truly have to.
- Some C-analogous Go types technically work in exported functions, but type substitutions can behave badly. `*C.char` is usually less surprising than trying to be clever.
- Go functions cannot be cast to or from `unsafe.Pointer`, which limits how "dynamic" the boundary can be.
- "Do it in Go" does not remove the C problem. Once you touch things like `<sys/stat.h>`, varargs, loader behavior, and libc symbol interposition, you are back in C ABI territory whether you want to be or not.

## Related context

There are not many substantial examples of `LD_PRELOAD`-driven filesystem-style work in Go. A useful C reference point is [ldpfuse](https://github.com/sholtrop/ldpfuse/). A commercial example of the broader technique is [cunoFS](https://cuno.io/).
