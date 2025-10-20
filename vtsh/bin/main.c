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
#include "runner.h"
#include "vtsh.h"

#define MAX_INPUT 256

static void reap_background(void) {
  int status = 0;
  while (waitpid(-1, &status, WNOHANG) > 0) {
  }
}

int main() {
  (void)setvbuf(stdin, NULL, _IONBF, 0);

  char input[MAX_INPUT];

  while (1) {
    printf("%s", vtsh_prompt());

    if (fgets(input, sizeof(input), stdin) == NULL) {
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
