package main

/*
#include <dlfcn.h>
#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>

typedef int (*open_t)(const char *pathname, int flags, ...);
typedef int (*open64_t)(const char *pathname, int flags, ...);
typedef FILE* (*fopen_t)(const char *pathname, const char *mode);

static open_t real_open = NULL;
static open64_t real_open64 = NULL;
static fopen_t real_fopen = NULL;

int open(const char *pathname, int flags, ...) {
    if (real_open == NULL) {
        real_open = (open_t)dlsym(RTLD_NEXT, "open");
    }

    va_list args;
    va_start(args, flags);
    int mode = va_arg(args, int);
    va_end(args);

    printf("Hello World from open\n");
    if (flags & O_CREAT) {
        return real_open(pathname, flags, mode);
    } else {
        return real_open(pathname, flags);
    }
}

int open64(const char *pathname, int flags, ...) {
    if (real_open64 == NULL) {
        real_open64 = (open64_t)dlsym(RTLD_NEXT, "open64");
    }

    va_list args;
    va_start(args, flags);
    int mode = va_arg(args, int);
    va_end(args);

    printf("Hello World from open64\n");
    if (flags & O_CREAT) {
        return real_open64(pathname, flags, mode);
    } else {
        return real_open64(pathname, flags);
    }
}

FILE* fopen(const char *pathname, const char *mode) {
    if (real_fopen == NULL) {
        real_fopen = (fopen_t)dlsym(RTLD_NEXT, "fopen");
    }

    printf("Hello World from fopen\n");
    return real_fopen(pathname, mode);
}
*/
import "C"

func main() {
	shell()
}
