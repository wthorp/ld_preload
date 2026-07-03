//go:build linux

package main

/*
#cgo linux LDFLAGS: -ldl -pthread
#include "rtld.h"
*/
import "C"
