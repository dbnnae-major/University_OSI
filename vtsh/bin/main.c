#include <stdio.h>
#include <string.h>
#include <vtsh.h>

#define MAX_INPUT 256
#define MAX_ARGS 8

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

    printf("%d", argc);
  }

  return 0;
}
