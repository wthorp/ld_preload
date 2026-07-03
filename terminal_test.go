package main

import (
	"errors"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"testing"

	"golang.org/x/sys/unix"
)

func resetTerminalGlobals() {
	currentEnv = os.Environ
	currentGetwd = os.Getwd
	currentShell = func() string { return os.Getenv("SHELL") }
	execCommand = exec.Command
	inputFile = os.Stdin
	outputFile = os.Stdout
	errorFile = os.Stderr
	makeRawFn = makeRaw
	restoreTerminalFn = restoreTerminal
}

func TestBuildShellCommandUsesShellEnvAndPreload(t *testing.T) {
	t.Cleanup(resetTerminalGlobals)

	tmpDir := t.TempDir()
	currentEnv = func() []string { return []string{"BASE=value"} }
	currentGetwd = func() (string, error) { return tmpDir, nil }
	currentShell = func() string { return "/bin/../bin/zsh" }

	cmd, err := buildShellCommand()
	if err != nil {
		t.Fatalf("buildShellCommand returned error: %v", err)
	}

	if got, want := cmd.Path, "/bin/zsh"; got != want {
		t.Fatalf("unexpected shell path: got %q want %q", got, want)
	}

	if len(cmd.Args) != 2 || cmd.Args[1] != "-i" {
		t.Fatalf("unexpected args: %#v", cmd.Args)
	}

	env := strings.Join(cmd.Env, "\n")
	if !strings.Contains(env, "BASE=value") {
		t.Fatalf("base env missing: %v", cmd.Env)
	}

	preload := "LD_PRELOAD=" + filepath.Join(tmpDir, "ld_preload.so")
	if !strings.Contains(env, preload) {
		t.Fatalf("ld preload env missing: %v", cmd.Env)
	}

	if !strings.Contains(env, `PROMPT_COMMAND=printf "[ldp] - "`) {
		t.Fatalf("prompt command missing: %v", cmd.Env)
	}
}

func TestBuildShellCommandDefaultsToBash(t *testing.T) {
	t.Cleanup(resetTerminalGlobals)

	currentEnv = func() []string { return nil }
	currentGetwd = func() (string, error) { return "/tmp", nil }
	currentShell = func() string { return "" }

	cmd, err := buildShellCommand()
	if err != nil {
		t.Fatalf("buildShellCommand returned error: %v", err)
	}

	if got, want := cmd.Path, "/bin/bash"; got != want {
		t.Fatalf("unexpected default shell path: got %q want %q", got, want)
	}
}

func TestBuildShellCommandReturnsGetwdError(t *testing.T) {
	t.Cleanup(resetTerminalGlobals)

	currentGetwd = func() (string, error) { return "", errors.New("boom") }

	_, err := buildShellCommand()
	if err == nil {
		t.Fatal("expected error from buildShellCommand")
	}

	if !strings.Contains(err.Error(), "get current directory") {
		t.Fatalf("unexpected error: %v", err)
	}
}

func TestShellReturnsWhenBuildFails(t *testing.T) {
	t.Cleanup(resetTerminalGlobals)

	currentGetwd = func() (string, error) { return "", errors.New("boom") }
	errorFile = tempOutputFile(t)

	shell()

	if got := readOutputFile(t, errorFile); !strings.Contains(got, "Error building shell command:") {
		t.Fatalf("expected build error message, got %q", got)
	}
}

func TestShellReturnsWhenMakeRawFails(t *testing.T) {
	t.Cleanup(resetTerminalGlobals)

	currentEnv = func() []string { return nil }
	currentGetwd = func() (string, error) { return "/tmp", nil }
	currentShell = func() string { return "/bin/sh" }
	errorFile = tempOutputFile(t)
	makeRawFn = func(*os.File) (*unix.Termios, error) {
		return nil, errors.New("raw mode failed")
	}

	shell()

	if got := readOutputFile(t, errorFile); !strings.Contains(got, "Error setting terminal to raw mode: raw mode failed") {
		t.Fatalf("expected makeRaw error message, got %q", got)
	}
}

func tempOutputFile(t *testing.T) *os.File {
	t.Helper()

	f, err := os.CreateTemp(t.TempDir(), "stderr-*")
	if err != nil {
		t.Fatalf("CreateTemp failed: %v", err)
	}
	t.Cleanup(func() { _ = f.Close() })
	return f
}

func readOutputFile(t *testing.T, f *os.File) string {
	t.Helper()

	if _, err := f.Seek(0, 0); err != nil {
		t.Fatalf("Seek failed: %v", err)
	}
	data, err := os.ReadFile(f.Name())
	if err != nil {
		t.Fatalf("ReadFile failed: %v", err)
	}
	return string(data)
}
