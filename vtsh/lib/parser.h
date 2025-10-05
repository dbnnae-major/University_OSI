#pragma once

#define MAX_CMDS  16      
#define MAX_ARGS  16      

typedef enum {
  OP_NONE,  // нет оператора
  OP_BG     // &
} Operator;

typedef struct Command {
  char** argv;
  Operator next_op;
} Command;

typedef struct {
  Command* commands;
  int count;
} ParsedInput;

ParsedInput parse_input(const char* input);

void free_memory(ParsedInput* input);

char* make_buf(const char* input);