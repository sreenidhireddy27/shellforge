# Shellforge

A modular, lightweight POSIX-compliant UNIX shell implemented in C (`gnu11`). Built incrementally to demonstrate core systems programming concepts, low-level process management, inter-process communication, and POSIX signal handling.

---

## Features

- **Process Lifecycle Management**: Command execution using `fork()`, `execvp()`, and status monitoring via `waitpid()`.
- **Built-in Commands**: Parent-process execution of built-ins including `cd` (via `chdir()`) and `history`.
- **I/O Redirection**: Standard output truncation (`>`) and standard input redirection (`<`) implemented with `open()` and `dup2()`.
- **Pipelines**: Unidirectional inter-process communication between processes using `pipe()` (`cmd1 | cmd2`).
- **Signal Handling**: Asynchronous `SIGINT` (Ctrl+C) interception via `sigaction` with safe foreground signal forwarding.
- **Asynchronous Execution & Zombie Reaping**: Background job execution with trailing `&` and automatic child cleanup via non-blocking `SIGCHLD` handlers (`WNOHANG`).
- **Variable Expansion**: In-place expansion of environment variables (`$VAR`) via `getenv()`.
- **Command History**: In-memory ring buffer tracking executed commands.
- **Modular Architecture**: Clean header separation (`include/shellforge.h`) and isolated modules for built-ins, execution, and the REPL.

---

## Project Structure

```text
shellforge/
├── include/
│   └── shellforge.h    # Shared declarations, constants, and function prototypes
├── src/
│   ├── builtins.c      # Built-in command routines (cd, history)
│   ├── executor.c      # Redirection, pipelines, and process execution
│   └── main.c          # REPL loop, tokenization, signal setup, variable expansion
├── Makefile            # Build configuration
└── README.md           # Project documentation
