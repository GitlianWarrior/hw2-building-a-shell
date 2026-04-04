// mysh - a simple Unix shell

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
  char line[1024];
  char *args[64];
  int arg_count;

  while (1) {
    char *stdout_filename = NULL;
    char *stdin_filename = NULL;
    char **consumer = NULL; // Command to the right of |
    printf("mysh> ");
    fflush(stdout); // Sends the prompt immediately

    // Handles ctrl + d
    if (fgets(line, sizeof(line), stdin) == NULL) {
      printf("\n");
      break;
    }

    // Finds the newline operator and replaces it with a null terminator
    char *newline = strchr(line, '\n');
    if (newline)
      *newline = '\0';

    // Parse Check
    arg_count = 0;
    args[arg_count] = strtok(line, " ");
    while (args[arg_count] != NULL) {
      arg_count++;
      args[arg_count] = strtok(NULL, " ");
    }

    // Empty Line Check
    if (arg_count == 0) {
      continue;
    }

    // Built In Command: cd
    if (strcmp(args[0], "cd") == 0) {
      if (args[1] == NULL) {
        if (chdir(getenv("HOME")) == -1) {
          perror("cd failed");
        }
      } else {
        if (chdir(args[1]) == -1) {
          perror("cd failed");
        }
      }
      continue;
    }

    // Built In Command: exit
    if (strcmp(args[0], "exit") == 0) {
      break;
    }

    // Pipe Finder
    for (int i = 0; i < arg_count; i++) {
      if (strcmp(args[i], "|") == 0) {
        consumer = &args[i + 1];
        args[i] = NULL;
      }
    }

    if (consumer != NULL) {
      int pipefd[2];
      if (pipe(pipefd) == -1) {
        perror("pipefd failed");
        exit(1);
      }
      pid_t left_pid = fork();
      if (left_pid == 0) { // First Child Process (Consumer)
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        execvp(args[0], args);
        perror("execvp failed");
        exit(1);
      }
      pid_t right_pid = fork();
      if (right_pid == 0) { // Second Child Process (Producer)
        close(pipefd[1]);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        execvp(consumer[0], consumer);
        perror("execvp failed");
        exit(1);
      }
      if (left_pid > 0 && right_pid > 0) { // Parent Process
        close(pipefd[0]);
        close(pipefd[1]);
        waitpid(left_pid, NULL, 0);
        waitpid(right_pid, NULL, 0);
      }

    } else {

      // Redirection Finder
      for (int i = 0; i < arg_count; i++) {
        if (strcmp(args[i], ">") == 0) {
          stdout_filename = args[i + 1];
          args[i] = NULL;
        } else if (strcmp(args[i], "<") == 0) {
          stdin_filename = args[i + 1];
          args[i] = NULL;
        }
      }

      pid_t pid = fork(); // Creates a new process
      if (pid == 0) {     // child (exec)
        // stdout
        if (stdout_filename != NULL) {
          int fd = open(stdout_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
          if (fd == -1) {
            perror("open failed");
            exit(1);
          } else {
            dup2(fd, STDOUT_FILENO);
            close(fd);
          }
        }

        // stdin
        if (stdin_filename != NULL) {
          int fd = open(stdin_filename, O_RDONLY);
          if (fd == -1) {
            perror("open failed");
            exit(1);
          } else {
            dup2(fd, STDIN_FILENO);
            close(fd);
          }
        }

        execvp(args[0], args);
        perror("execvp failed");
        exit(1);
      } else if (pid > 0) { // parent (wait)
        waitpid(pid, NULL, 0);
      } else { // negative (error)
        perror("fork failed");
      }

      /*
        Test:
        printf("You entered: %s\n", line);
       for (int i = 0; i < arg_count; i++) {
         printf("Token %d: %s\n", i, args[i]);
       }
       */
    }
  }
  return 0;
}
