#include "runner.h"

#include <limits.h>
#include <linux/sched.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define E9 1000000000.0

static int get_self_path(char* buf, size_t n) {
  ssize_t read = readlink("/proc/self/exe", buf, n - 1);
  if (read < 0 || (size_t)read >= n) {
    return -1;
  }
  buf[read] = '\0';
  return 0;
}

static int is_interactive(void) {
  return isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
}

void run_command(char* argv[], int background) {
  struct timespec start;
  struct timespec end;
  struct clone_args args;

  int is_self = (argv && argv[0] && (!strcmp(argv[0], "./shell")));

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
    if (is_self) {
      char self[PATH_MAX];
      if (get_self_path(self, sizeof self) == 0) {
        execv(self, argv);
      }
    }
    execvp(argv[0], argv);
    if (!is_interactive()) {
      const char* msg = "Command not found\n";
      (void)!write(STDOUT_FILENO, msg, strlen(msg));
    } else {
      perror("execvp failed");
    }
    _exit(1);
  }

  if (background) {
    if (is_interactive()) {
      printf("[bg] pid=%d: %s\n", pid, argv[0]);
    }
    return;
  }

  waitpid(pid, NULL, 0);
  clock_gettime(CLOCK_MONOTONIC, &end);
  double time = (double)(end.tv_sec - start.tv_sec) +
                (double)(end.tv_nsec - start.tv_nsec) / E9;

  if (is_interactive()) {
    printf("Execution time: %.6f sec\n", time);
  }
}
