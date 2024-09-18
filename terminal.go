package main

import (
	"fmt"
	"os"
	"os/exec"
)

func shell() {
	// Get the current environment variables
	env := os.Environ()

	// Append LD_PRELOAD flags to the environment
	preload := "ld_preload.so"
	// Get the current working directory
	dir, err := os.Getwd()
	if err != nil {
		fmt.Println("Error getting the current directory:", err)
		return
	}
	env = append(env, "LD_PRELOAD="+dir+"/"+preload)
	env = append(env, "PROMPT_COMMAND=printf \"[ldp] - \"")

	// Define the shell to execute
	shell := os.Getenv("SHELL")
	if shell == "" {
		shell = "/bin/bash"
	}

	// Run the shell with the modified environment
	cmd := exec.Command(shell, "-i")
	cmd.Env = env
	cmd.Stdin = os.Stdin
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr

	// Set terminal to raw mode
	oldState, err := makeRaw(os.Stdin)
	if err != nil {
		fmt.Println("Error setting terminal to raw mode:", err)
		return
	}
	defer restoreTerminal(os.Stdin, oldState)

	// Start the shell process
	if err := cmd.Start(); err != nil {
		fmt.Println("Error starting shell:", err)
		return
	}

	// Wait for the shell process to finish
	if err := cmd.Wait(); err != nil {
		fmt.Println("Error waiting for shell:", err)
		return
	}

}
