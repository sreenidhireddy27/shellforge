#ifndef SHELLFORGE_H
#define SHELLFORGE_H

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_TOKENS 64
#define DELIMITERS " \t\r\n\a"
#define MAX_HISTORY 100

extern pid_t foreground_pid;

// Built-in commands & history management
void add_history_entry(const char *cmd);
int shellforge_history(void);
void free_history(void);
int shellforge_cd(char **args);

// Execution & redirection
void handle_redirection(char **args);
int execute_pipeline(char **args1, char **args2);
int execute_external(char **args, int in_background);
int execute_command(char **args);

#endif // SHELLFORGE_H
