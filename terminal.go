package main

import (
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
)

var (
	currentEnv        = os.Environ
	currentGetwd      = os.Getwd
	currentShell      = func() string { return os.Getenv("SHELL") }
	execCommand       = exec.Command
	inputFile         = os.Stdin
	outputFile        = os.Stdout
	errorFile         = os.Stderr
	makeRawFn         = makeRaw
	restoreTerminalFn = restoreTerminal
)

func writeShellError(args ...any) {
	_, _ = fmt.Fprintln(errorFile, args...)
}

func buildShellCommand() (*exec.Cmd, error) {
	env := currentEnv()

	dir, err := currentGetwd()
	if err != nil {
		return nil, fmt.Errorf("get current directory: %w", err)
	}

	env = append(env, "LD_PRELOAD="+filepath.Join(dir, "ld_preload.so"))
	env = append(env, "PROMPT_COMMAND=printf \"[ldp] - \"")

	shell := currentShell()
	if shell == "" {
		shell = "/bin/bash"
	}
	shell = filepath.Clean(shell)

	// #nosec G204: the shell path comes from the user environment and is normalized before exec.
	cmd := execCommand(shell, "-i")
	cmd.Env = env
	cmd.Stdin = inputFile
	cmd.Stdout = outputFile
	cmd.Stderr = errorFile

	return cmd, nil
}

func shell() {
	cmd, err := buildShellCommand()
	if err != nil {
		writeShellError("Error building shell command:", err)
		return
	}

	// Set terminal to raw mode
	oldState, err := makeRawFn(inputFile)
	if err != nil {
		writeShellError("Error setting terminal to raw mode:", err)
		return
	}
	defer restoreTerminalFn(inputFile, oldState)

	// Start the shell process
	if err := cmd.Start(); err != nil {
		writeShellError("Error starting shell:", err)
		return
	}

	// Wait for the shell process to finish
	if err := cmd.Wait(); err != nil {
		writeShellError("Error waiting for shell:", err)
		return
	}
}
