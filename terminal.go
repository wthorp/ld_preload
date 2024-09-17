package main

import (
	"fmt"
	"os"
	"os/exec"
	"strings"
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

	// Set the PS1 environment variable to change the shell prompt
	newPS1 := "(preload) %n@%m %1~ %# "
	foundPS1 := false
	for i, e := range env {
		if strings.HasPrefix(e, "PS1=") {
			env[i] = "PS1=" + newPS1
			foundPS1 = true
			break
		}
	}

	// If PS1 is not found in the environment, add it
	if !foundPS1 {
		env = append(env, "PS1="+newPS1)
	}

	// Define the shell to execute
	shell := os.Getenv("SHELL")
	if shell == "" {
		shell = "/bin/bash" // default to /bin/bash if SHELL environment variable is not set
	}

	// Run the shell with the modified environment
	cmd := exec.Command(shell, "-fi") //todo:  use correct flags across shells to skip init scripts
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
