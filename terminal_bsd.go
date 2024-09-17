//go:build darwin || freebsd || openbsd || netbsd
// +build darwin freebsd openbsd netbsd

package main

import (
	"os"

	"golang.org/x/sys/unix"
)

// makeRaw sets the terminal to raw mode on BSD and macOS.
func makeRaw(f *os.File) (*unix.Termios, error) {
	oldState, err := unix.IoctlGetTermios(int(f.Fd()), unix.TIOCGETA)
	if err != nil {
		return nil, err
	}
	newState := *oldState
	newState.Lflag &^= unix.ECHO | unix.ICANON
	newState.Cc[unix.VMIN] = 1
	newState.Cc[unix.VTIME] = 0
	if err := unix.IoctlSetTermios(int(f.Fd()), unix.TIOCSETA, &newState); err != nil {
		return nil, err
	}
	return oldState, nil
}

// restoreTerminal restores the terminal to its previous state on BSD and macOS.
func restoreTerminal(f *os.File, oldState *unix.Termios) {
	unix.IoctlSetTermios(int(f.Fd()), unix.TIOCSETA, oldState)
}
