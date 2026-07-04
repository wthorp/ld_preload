# ld_preload

Prototype Linux `LD_PRELOAD` interposer for file-oriented libc calls, with Go used around the edges rather than in the hot interception path.

It started as Go code but has morphed to mostly C, mostly because this is no job for cgo's generated declarations.

## What this is

This project explores a FUSE-like idea: intercept common file I/O entry points in a shared object and decide in user space how those calls should behave.

If you want to understand the context, [To FUSE or Not to FUSE](https://www.usenix.org/conference/fast17/technical-sessions/presentation/vangoor) analyzes the cost of the kernel/user FUSE hop. [Defusing FUSE](https://www.osti.gov/servlets/purl/1458703) focuses on cutting that overhead by removing parts of the FUSE stack, and [later work](https://dl.acm.org/doi/10.1145/3494556) shows how to hide direct access behind a compatibility layer.

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

## On the design

Earlier attempts in this repo tried to override libc symbols with exported Go functions and then fulfill the work with Go syscall wrappers. That looked appealing on paper and worked for trivial cases, but it produced deadlocks in real processes.

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

Example rewrite policy:

```sh
LD_PRELOAD=/path/to/ld_preload.so \
LD_PRELOAD_REWRITE_FROM=/original/prefix \
LD_PRELOAD_REWRITE_TO=/replacement/prefix \
/bin/ls /original/prefix
```

## Rewrite Contract

The current rewrite layer is intentionally narrow.

It supports one lexical prefix mapping, configured by:

- `LD_PRELOAD_REWRITE_FROM`
- `LD_PRELOAD_REWRITE_TO`

Both values must be set, non-empty, absolute, and lexically cleanable. Invalid config disables rewriting silently. The process snapshots the config once on first use; later environment changes do not affect the running process.

Matching is lexical, not `realpath`-based:

- `/a/b` matches `/a/b` and `/a/b/x`
- `/a/b` does not match `/a/b2`

Path handling rules:

- absolute paths are cleaned and matched directly
- relative paths with `AT_FDCWD` resolve against `getcwd()`
- relative paths with a real `dirfd` resolve through `/proc/self/fd/<dirfd>`
- if resolution fails, path cleaning fails, buffers overflow, or the situation is uncertain, the call falls through unchanged

## Supported Core Surface

The defended rewrite surface is the file-tree illusion core:

- creation and open: `creat`, `open`, `openat`
- metadata lookup: `access`, `euidaccess`, `stat`, `lstat`, `statx`, `__xstat`, `__lxstat`, `__fxstatat`, `newfstatat`
- path changes: `rename`, `renameat`, `renameat2`, `link`, `linkat`, `symlink`, `symlinkat`, `unlink`, `unlinkat`, `mkdir`, `mkdirat`, `truncate`, `utimensat`
- open-family: `open`, `openat`, `openat2`
- access and mode queries: `faccessat`, `fchmodat`
- mount-like/helpers: `statfs`, `chdir`, `getcwd`
- process helper: `execve`
- symlink reads: `readlink`, `readlinkat`

Fd-only operations are pass-through:

- `close`, `fstat`, `lseek`, `pread`, `pwrite`, `read`, `write`

For `symlink` and `symlinkat`, the link path is rewritten, and the target is rewritten only when it is absolute. Relative symlink payloads are preserved.

## Deliberate Exclusions

This prototype does not currently try to cover every libc or kernel-facing file API.

In particular, it does not promise rewrite coverage for:

- xattr enumeration and mutation
- special-file creation such as `mknod`
- library-level path helpers such as `realpath`, `glob`, `opendir`, or traversal frameworks

Fd-only behavior and non-libc syscall paths are intentionally left as passthrough unless the behavior is explicitly modeled.

For audit, run `make docker-audit` to execute candidate workloads under `strace=%file` and emit:

- `observed-syscalls.txt`
- `rewrite-syscalls.txt`
- `passthrough-syscalls.txt`
- `missed-syscalls.txt`
- `syscall-summary.txt`

Those omissions are deliberate. The project is optimizing for a smaller, more defensible preload boundary rather than broad filesystem emulation.

## Fail-Open Behavior

The library is designed to fail open.

If config is invalid, symbols are unavailable, path resolution fails, `/proc/self/fd` lookup fails, `getcwd()` is unusable, or the helper layer cannot safely produce a rewritten path, the interposer forwards the original libc call unchanged.

## The pains of cgo

These are still the most relevant lessons from working on this:

- It is very easy to get errors like `conflicting types for 'fstat'` when the cgo-generated declarations do not match the C ABI exactly.
- Go structs cannot be used directly from C exports. In practice that means using C types at the boundary and converting with `unsafe.Pointer` only when you truly have to.
- Some C-analogous Go types technically work in exported functions, but type substitutions can behave badly. `*C.char` is usually less surprising than trying to be clever.
- Go functions cannot be cast to or from `unsafe.Pointer`, which limits how "dynamic" the boundary can be.
- "Do it in Go" does not remove the C problem. Once you touch things like `<sys/stat.h>`, varargs, loader behavior, and libc symbol interposition, you are back in C ABI territory whether you want to be or not.

## Related context

There are not many substantial examples of `LD_PRELOAD`-driven filesystem-style work in Go. A useful C reference point is [ldpfuse](https://github.com/sholtrop/ldpfuse/). A commercial example of the broader technique is [cunoFS](https://cuno.io/).
