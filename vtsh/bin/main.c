#include <linux/sched.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "parser.h"
#include "vtsh.h"

#define MAX_INPUT 256
#define E9 1000000000.0

static void reap_background(void) {
  int status = 0;
  while (waitpid(-1, &status, WNOHANG) > 0) {
  }
}

void run_command(char* argv[], int background) {
  struct timespec start;
  struct timespec end;
  struct clone_args args;

  memset(&args, 0, sizeof(args));
  args.exit_signal = SIGCHLD;
  pid_t pid = -1;

  if (!background) {
    clock_gettime(CLOCK_MONOTONIC, &start);
  }
  pid = (pid_t)syscall(SYS_clone3, &args, sizeof(args));

  if (pid == -1) {
    perror("clone3 failed");
    return;
  }
  if (pid == 0) {
    execvp(argv[0], argv);
    perror("execvp failed");
    _exit(1);
  }

  if (background) {
    printf("[bg] pid=%d: %s\n", pid, argv[0]);
    return;
  }

  waitpid(pid, NULL, 0);
  clock_gettime(CLOCK_MONOTONIC, &end);
  double time = (double)(end.tv_sec - start.tv_sec) +
                (double)(end.tv_nsec - start.tv_nsec) / E9;
  printf("Execution time: %.6f sec\n", time);
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

    ParsedInput parsed_input = parse_input(input);
    if (parsed_input.count == 0) {
      free_memory(&parsed_input);
      continue;
    }

    for (int i = 0; i < parsed_input.count; ++i) {
      Command* command = &parsed_input.commands[i];
      if (!command->argv || !command->argv[0]) {
        continue;
      }

      int background = (command->next_op == OP_BG);
      run_command(command->argv, background);
    }

    reap_background();

    free_memory(&parsed_input);
  }

  return 0;
}
