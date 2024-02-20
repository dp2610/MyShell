#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <fnmatch.h>
#include <glob.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/stat.h>

#define MAX_COMMAND_LENGTH 1024
#define MAX_ARGS 64
#define FILE_MODE 0640

// Function declarations
void interactive_mode();
void batch_mode(char* filename);
void execute_command(char* command);
char** parse_command(char* command, int* num_args, int* is_background, char** redirect_in, char** redirect_out);
void expand_wildcards(char*** args, int* num_args);
void handle_redirection(char** args, int num_args, char* redirect_in, char* redirect_out);
void handle_pipe(char** args1, char** args2);
int handle_builtins(char** args);
int handle_conditionals(char* command);
void which_command(char* command);

int main(int argc, char *argv[]) {
    // Main function to start the shell in either interactive or batch mode
    if (argc > 1) {
        batch_mode(argv[1]);
    } else {
        interactive_mode();
    }
    return 0;
}

void interactive_mode() {
    char command[MAX_COMMAND_LENGTH];
    printf("Welcome to my shell!\n");
    while (1) {
        printf("mysh> ");
        if (fgets(command, MAX_COMMAND_LENGTH, stdin) == NULL) {
            // Handles EOF and potential reading errors
            if (feof(stdin)) {
                printf("mysh: exiting\n");  
                break; 
            } else {
                perror("Error reading input");
                continue;
            }
        }

        if (command[strlen(command) - 1] == '\n') {
            command[strlen(command) - 1] = 0;
        }

        // Check for 'exit' command
        if (strcmp(command, "exit") == 0) {
            printf("mysh: exiting\n"); 
            break;
        }

        execute_command(command);
    }
}

void batch_mode(char* filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    char command[MAX_COMMAND_LENGTH];
    while (fgets(command, MAX_COMMAND_LENGTH, file) != NULL) {
        if (command[strlen(command) - 1] == '\n') {
            command[strlen(command) - 1] = 0;
        }
        execute_command(command);
    }
    fclose(file);
}

int last_command_status = 0;  // Global variable to store the exit status of the last command

void execute_command(char* command) {
    // Conditional command handling
    if (strncmp(command, "then ", 5) == 0) {
        if (last_command_status == 0) {
            execute_command(command + 5);  
        }
        return;
    } else if (strncmp(command, "else ", 5) == 0) {
        if (last_command_status != 0) {
            execute_command(command + 5);  
        }
        return;
    }
    
    // Check for pipe in the command
    char* pipe_pos = strchr(command, '|');
    if (pipe_pos != NULL) {
        *pipe_pos = '\0'; // Split the command at the pipe symbol
        char* command1 = command;
        char* command2 = pipe_pos + 1;

        int pipe_fds[2];
        pipe(pipe_fds);

        // First child process
        if (fork() == 0) { 
            dup2(pipe_fds[1], STDOUT_FILENO);
            close(pipe_fds[0]);
            close(pipe_fds[1]);
            execute_command(command1);
            exit(EXIT_SUCCESS);
        }

        // Second child process
        if (fork() == 0) { 
            dup2(pipe_fds[0], STDIN_FILENO);
            close(pipe_fds[1]);
            close(pipe_fds[0]);
            execute_command(command2);
            exit(EXIT_SUCCESS);
        }

        close(pipe_fds[0]);
        close(pipe_fds[1]);
        wait(NULL);
        wait(NULL);
        return;
    }

    // Parsing the command
    int num_args, is_background;
    char* redirect_in = NULL;
    char* redirect_out = NULL;
    char** args = parse_command(command, &num_args, &is_background, &redirect_in, &redirect_out);

    if (handle_builtins(args) == 0) {
        int pid = fork();
        // Child process
        if (pid == 0) { 
            // Handle redirection
            if (redirect_in != NULL) {
                int fd_in = open(redirect_in, O_RDONLY);
                if (fd_in < 0) {
                    perror("Error opening input file");
                    exit(EXIT_FAILURE);
                }
                dup2(fd_in, STDIN_FILENO);
                close(fd_in);
            }

            if (redirect_out != NULL) {
                int fd_out = open(redirect_out, O_WRONLY | O_CREAT | O_TRUNC, FILE_MODE);
                if (fd_out < 0) {
                    perror("Error opening output file");
                    exit(EXIT_FAILURE);
                }
                dup2(fd_out, STDOUT_FILENO);
                close(fd_out);
            }

            // Execute the command
            execvp(args[0], args);
            perror("execvp failed");
            exit(EXIT_FAILURE);

        } else if (pid > 0) { // Parent process
            if (!is_background) {
                int status;
                waitpid(pid, &status, 0);
                last_command_status = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
            }
        } else {
            perror("fork");
            exit(EXIT_FAILURE);
        }
    }

    // Free allocated memory
    for (int i = 0; i < num_args; i++) {
        free(args[i]);
    }
    free(args);

    if (redirect_in) free(redirect_in);
    if (redirect_out) free(redirect_out);
}

char** parse_command(char* command, int* num_args, int* is_background, char** redirect_in, char** redirect_out) {
    char** args = malloc(MAX_ARGS * sizeof(char*));
    *num_args = 0;
    *is_background = 0;
    *redirect_in = NULL;
    *redirect_out = NULL;
    char* token = strtok(command, " ");

    while (token != NULL) {
        if (strcmp(token, "<") == 0) {
            token = strtok(NULL, " ");
            *redirect_in = strdup(token);
        } else if (strcmp(token, ">") == 0) {
            token = strtok(NULL, " ");
            *redirect_out = strdup(token);
        } else if (strcmp(token, "&") == 0) {
            *is_background = 1;
        } else {
            args[*num_args] = strdup(token);
            (*num_args)++;
        }
        token = strtok(NULL, " ");
    }

    expand_wildcards(&args, num_args);

    return args;
}

void expand_wildcards(char*** args, int* num_args) {
    glob_t glob_result;
    memset(&glob_result, 0, sizeof(glob_result));
    char** new_args = malloc(MAX_ARGS * sizeof(char*));
    int glob_index = 0;

    for (int i = 0; i < *num_args; i++) {
        if (strpbrk((*args)[i], "*?") != NULL) {
            glob((*args)[i], GLOB_NOCHECK | GLOB_TILDE, NULL, &glob_result);
            for (int j = 0; j < glob_result.gl_pathc; j++) {
                new_args[glob_index++] = strdup(glob_result.gl_pathv[j]);
            }
        } else {
            new_args[glob_index++] = strdup((*args)[i]);
        }
        free((*args)[i]); 
    }

    free(*args); // Free the old args array
    *args = new_args; // Update the pointer with the new array
    *num_args = glob_index;
    globfree(&glob_result);
}

void handle_redirection(char** args, int num_args, char* redirect_in, char* redirect_out) {
    int orig_stdin = dup(STDIN_FILENO);
    int orig_stdout = dup(STDOUT_FILENO);

    if (redirect_in != NULL) {
        int fd_in = open(redirect_in, O_RDONLY);
        if (fd_in < 0) {
            perror("Error opening input file");
            exit(EXIT_FAILURE);
        }
        dup2(fd_in, STDIN_FILENO);
        close(fd_in);
    }

    if (redirect_out != NULL) {
        int fd_out = open(redirect_out, O_WRONLY | O_CREAT | O_TRUNC, FILE_MODE);
        if (fd_out < 0) {
            perror("Error opening output file");
            exit(EXIT_FAILURE);
        }
        dup2(fd_out, STDOUT_FILENO);
        close(fd_out);
    }

    // After command execution this will restore the original file descriptors
    dup2(orig_stdin, STDIN_FILENO);
    close(orig_stdin);
    dup2(orig_stdout, STDOUT_FILENO);
    close(orig_stdout);
}

void handle_pipe(char** args1, char** args2) {
    int fd[2];
    if (pipe(fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    int pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);
        execvp(args1[0], args1);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        close(fd[1]);
        int status;
        waitpid(pid, &status, 0);  
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);
        execvp(args2[0], args2);
        perror("execvp");
        exit(EXIT_FAILURE);
    }
}

int handle_builtins(char** args) {
    if (strcmp(args[0], "cd") == 0) {
        // If no argument is provided with 'cd' set default to the home directory
        const char* directory = args[1] ? args[1] : getenv("HOME");
        if (chdir(directory) != 0) {
            perror("cd");
        }
        return 1;
    } else if (strcmp(args[0], "pwd") == 0) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            perror("pwd");
        }
        return 1;
    } else if (strcmp(args[0], "exit") == 0) {
        exit(0);
    } else if (strcmp(args[0], "which") == 0) {
        which_command(args[1]);
        return 1;
    }
    return 0; 
}

void which_command(char* command) {
    char* path_env = strdup(getenv("PATH")); 
    char* token;
    char* rest = path_env;
    char executable_path[MAX_COMMAND_LENGTH];
    struct stat st;

    while ((token = strtok_r(rest, ":", &rest))) {
        sprintf(executable_path, "%s/%s", token, command);
        if (stat(executable_path, &st) == 0 && st.st_mode & S_IXUSR) {
            printf("%s\n", executable_path); 
            free(path_env);
            return;
        }
    }

    printf("Command not found: %s\n", command);
    free(path_env); 
}

int handle_conditionals(char* command) {
    char pre_conditional[MAX_COMMAND_LENGTH] = {0};
    char* then_part = strstr(command, " then ");
    char* else_part = strstr(command, " else ");
    int status = 0;

    if (then_part != NULL) {
        strncpy(pre_conditional, command, then_part - command);
        pre_conditional[then_part - command] = '\0';
        execute_command(pre_conditional);
        wait(&status);

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            execute_command(then_part + 5); 
        }
    } else if (else_part != NULL) {
        strncpy(pre_conditional, command, else_part - command);
        pre_conditional[else_part - command] = '\0';
        execute_command(pre_conditional);
        wait(&status);

        if (!(WIFEXITED(status) && WEXITSTATUS(status) == 0)) {
            execute_command(else_part + 5); 
        }
    }

    return 1; 
}