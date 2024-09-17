//go:build linux
// +build linux

package main

import (
	"fmt"
	"os"

	"golang.org/x/sys/unix"
)

// makeRaw sets the terminal to raw mode.
func makeRaw(f *os.File) (*unix.Termios, error) {
	oldState, err := unix.IoctlGetTermios(int(f.Fd()), unix.TCGETS)
	if err != nil {
		return nil, fmt.Errorf("ioctl get termios: %v", err)
	}

	// newState := *oldState
	// newState.Lflag &^= unix.ECHO | unix.ICANON | unix.ISIG
	// newState.Iflag &^= unix.IXON | unix.ICRNL
	// newState.Cc[unix.VMIN] = 1
	// newState.Cc[unix.VTIME] = 0

	// if err := unix.IoctlSetTermios(int(f.Fd()), unix.TCSETS, &newState); err != nil {
	// 	return nil, fmt.Errorf("ioctl set termios: %v", err)
	// }

	return oldState, nil
}

// restoreTerminal restores the terminal to its previous state.
func restoreTerminal(f *os.File, oldState *unix.Termios) {
	// if err := unix.IoctlSetTermios(int(f.Fd()), unix.TCSETS, oldState); err != nil {
	// 	fmt.Fprintf(os.Stderr, "ioctl restore termios: %v\n", err)
	// }
}
