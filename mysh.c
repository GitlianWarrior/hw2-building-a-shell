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
    char *filename = NULL;
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

    // > Finder
    for (int i = 0; i < arg_count; i++) {
      if (strcmp(args[i], ">") == 0) {
        filename = args[i + 1];
        args[i] = NULL;
      }
    }

    pid_t pid = fork(); // Creates a new process

    if (pid == 0) { // child (exec)

      if (filename != NULL) {
        int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
          perror("open failed");
          exit(1);
        } else {
          dup2(fd, STDOUT_FILENO);
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
  return 0;
}
