#include <stdio.h>
#include <string.h>
#include <vtsh.h>

#define MAX_INPUT 256

int main() {
  char input[MAX_INPUT];

  while (1) {
    printf("%s", vtsh_prompt());

    if (fgets(input, sizeof(input), stdin) == NULL) {
      printf("Error of input\n");
      break;
    }

    input[strcspn(input, "\n")] = '\0';

    if (strcspn(input, "exit") == 0) {
      printf("%s", vtsh_exit_prompt());
      break;
    }
  }

  return 0;
}
