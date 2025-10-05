#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* make_buf(const char* input) {
  size_t length = strlen(input) + 1;
  char* buf = (char*)malloc(length);
  if (!buf) {
    return NULL;
  }
  memcpy(buf, input, length);
  return buf;
}

void free_memory(ParsedInput* input) {
  if (!input || !input->commands) {
    return;
  }
  for (int i = 0; i < input->count; ++i) {
    if (!input->commands[i].argv) {
      continue;
    }
    for (int j = 0; input->commands[i].argv[j] != NULL; ++j) {
      free(input->commands[i].argv[j]);
    }
    free(input->commands[i].argv);
  }
  free(input->commands);
  input->commands = NULL;
  input->count = 0;
}

ParsedInput parse_input(const char* input) {
  ParsedInput out = {
      .commands = (Command*)calloc(MAX_CMDS, sizeof(Command)), .count = 0
  };

  for (int i = 0; i < MAX_CMDS; ++i) {
    out.commands[i].argv = (char**)calloc(MAX_ARGS, sizeof(char*));
  }

  char* buf = make_buf(input);
  char* save = NULL;
  char* frag = strtok_r(buf, " \t", &save);

  int count = 0;  // индекс текущей команды
  int argc = 0;  // число аргументов в текущей команде

  while (frag != NULL) {
    if (strcmp(frag, "&") != 0) {
      if (argc < MAX_ARGS - 1) {
        out.commands[count].argv[argc++] = make_buf(frag);
      }
    } else {
      if (argc > 0) {
        out.commands[count].argv[argc] = NULL;
        out.commands[count].next_op = OP_BG;

        if (count + 1 < MAX_CMDS) {
          count++;
          argc = 0;
        } else {
          break;
        }
      }
    }
    frag = strtok_r(NULL, " \t", &save);
  }
  if (argc > 0) {
    out.commands[count].argv[argc] = NULL;
    out.commands[count].next_op = OP_NONE;
    count++;
  }
  out.count = count;

  free(buf);
  return out;
}