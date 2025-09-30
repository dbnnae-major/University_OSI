#include <linux/sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <vtsh.h>

#define MAX_INPUT 256
#define MAX_ARGS 8
#define E9 1000000000.0
#define STACK_SIZE ((size_t)(1024 * 1024))

int parse_input(char* input, char* argv[]) {
  int argc = 0;

  char* save = NULL;
  char* frag = strtok_r(input, " ", &save);

  while (frag != NULL && argc < MAX_ARGS - 1) {
    argv[argc++] = frag;
    frag = strtok_r(NULL, " ", &save);
  }

  argv[argc] = NULL;
  return argc;
}

void run_command(char* argv[]) {
  struct timespec start;
  struct timespec end;
  struct clone_args args;

  char child_stack[STACK_SIZE];

  memset(&args, 0, sizeof(args));
  args.flags = SIGCHLD;
  args.stack = (unsigned long)(child_stack + STACK_SIZE);
  args.stack_size = STACK_SIZE;
  pid_t pid = -1;

  clock_gettime(CLOCK_MONOTONIC, &start);
  pid = (pid_t)syscall(SYS_clone3, &args, sizeof(args));

  if (pid == -1) {
    perror("clone3 failed");
    return;
  }
  if (pid == 0) {
    execvp(argv[0], argv);
    perror("execvp failed");
    _exit(1);
  } else {
    waitpid(pid, NULL, 0);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double time = (double)(end.tv_sec - start.tv_sec) +
                  (double)(end.tv_nsec - start.tv_nsec) / E9;
    printf("Execution time: %.6f sec\n", time);
  }
}

int main() {
  char input[MAX_INPUT];

  while (1) {
    printf("%s", vtsh_prompt());

    if (fgets(input, sizeof(input), stdin) == NULL) {
      printf("Error of input\n");
      break;
    }

    input[strcspn(input, "\n")] = '\0';

    if (strcmp(input, "exit") == 0) {
      printf("%s", vtsh_exit_prompt());
      break;
    }

    char* argv[MAX_ARGS];
    int argc = parse_input(input, argv);

    if (argc == 0) {
      continue;
    }

    run_command(argv);
  }

  return 0;
}
