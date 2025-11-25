#define _GNU_SOURCE
#include "args.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  off_t start_byte;
  off_t end_byte;
} ByteRange;

static const int kBaseDecimal = 10;
static const int kExpectedParamCount = 7;

static int parse_range_string_into(
    const char* input_string, ByteRange* out_range
) {
  const char* dash_ptr = strchr(input_string, '-');
  if (dash_ptr == NULL) {
    return -1;
  }

  size_t first_len = (size_t)(dash_ptr - input_string);
  size_t second_len = strlen(dash_ptr + 1);

  char* first_buf = (char*)malloc(first_len + 1);
  char* second_buf = (char*)malloc(second_len + 1);
  if (first_buf == NULL || second_buf == NULL) {
    free(first_buf);
    free(second_buf);
    return -1;
  }

  (void)memcpy(first_buf, input_string, first_len);
  first_buf[first_len] = '\0';
  (void)memcpy(second_buf, dash_ptr + 1, second_len);
  second_buf[second_len] = '\0';

  errno = 0;
  unsigned long long start_val = strtoull(first_buf, NULL, kBaseDecimal);
  unsigned long long end_val = strtoull(second_buf, NULL, kBaseDecimal);

  free(first_buf);
  free(second_buf);

  if (errno != 0) {
    return -1;
  }

  out_range->start_byte = (off_t)start_val;
  out_range->end_byte = (off_t)end_val;
  return 0;
}

void print_usage(const char* message) {
  if (message != NULL) {
    (void)fprintf(stderr, "error: %s\n", message);
  }
  (void)fprintf(
      stderr,
      "usage: ioload rw=read|write block_size=<bytes> block_count=<n> "
      "file=<path> "
      "range=<start>-<end> direct=on|off type=sequence|random\n"
  );
}

static int handle_param_rw(Config* config_out, const char* value_str) {
  if (strcmp(value_str, "read") == 0) {
    config_out->mode_read_write = MODE_READ;
    return 0;
  }
  if (strcmp(value_str, "write") == 0) {
    config_out->mode_read_write = MODE_WRITE;
    return 0;
  }
  print_usage("rw must be 'read' or 'write'");
  return -1;
}

static int handle_param_block_size(Config* config_out, const char* value_str) {
  errno = 0;
  unsigned long long parsed = strtoull(value_str, NULL, kBaseDecimal);
  if (errno != 0 || parsed == 0ULL) {
    print_usage("block_size must be positive");
    return -1;
  }
  config_out->block_size_bytes = (size_t)parsed;
  return 0;
}

static int handle_param_block_count(Config* config_out, const char* value_str) {
  errno = 0;
  unsigned long long parsed = strtoull(value_str, NULL, kBaseDecimal);
  if (errno != 0 || parsed == 0ULL) {
    print_usage("block_count must be positive");
    return -1;
  }
  config_out->block_count_total = parsed;
  return 0;
}

static int handle_param_file(Config* config_out, const char* value_str) {
  config_out->file_path = value_str;
  if (*(config_out->file_path) == '\0') {
    print_usage("file path empty");
    return -1;
  }
  return 0;
}

static int handle_param_range(Config* config_out, const char* value_str) {
  ByteRange byte_range;
  if (parse_range_string_into(value_str, &byte_range) != 0 ||
      byte_range.start_byte < 0 || byte_range.end_byte < 0) {
    print_usage("invalid range format");
    return -1;
  }
  if (byte_range.end_byte != 0 && byte_range.end_byte < byte_range.start_byte) {
    print_usage("range end < start");
    return -1;
  }
  config_out->range_start_byte = byte_range.start_byte;
  config_out->range_end_byte = byte_range.end_byte;
  return 0;
}

static int handle_param_direct(Config* config_out, const char* value_str) {
  if (strcmp(value_str, "on") == 0) {
    config_out->use_direct_io = 1;
    return 0;
  }
  if (strcmp(value_str, "off") == 0) {
    config_out->use_direct_io = 0;
    return 0;
  }
  print_usage("direct must be 'on' or 'off'");
  return -1;
}

static int handle_param_type(Config* config_out, const char* value_str) {
  if (strcmp(value_str, "sequence") == 0) {
    config_out->selection_mode = SEL_SEQUENCE;
    return 0;
  }
  if (strcmp(value_str, "random") == 0) {
    config_out->selection_mode = SEL_RANDOM;
    return 0;
  }
  print_usage("type must be 'sequence' or 'random'");
  return -1;
}

typedef int (*ParamHandlerFn)(Config*, const char*);

typedef struct {
  const char* key_name;
  ParamHandlerFn handler_fn;
} ParamHandler;

static const ParamHandler kParamHandlers[] = {
    {         "rw",          handle_param_rw},
    { "block_size",  handle_param_block_size},
    {"block_count", handle_param_block_count},
    {       "file",        handle_param_file},
    {      "range",       handle_param_range},
    {     "direct",      handle_param_direct},
    {       "type",        handle_param_type},
};

static int apply_param(
    Config* config_out, const char* key_str, const char* value_str
) {
  size_t handlers_count = sizeof(kParamHandlers) / sizeof(kParamHandlers[0]);
  for (size_t handler_index = 0; handler_index < handlers_count;
       ++handler_index) {
    if (strcmp(key_str, kParamHandlers[handler_index].key_name) == 0) {
      return kParamHandlers[handler_index].handler_fn(config_out, value_str);
    }
  }
  print_usage("unknown parameter");
  return -1;
}

int parse_args(int argc, char** argv, Config* config_out) {
  if (argc != (1 + kExpectedParamCount)) {
    print_usage("expected 7 parameters");
    return -1;
  }

  config_out->mode_read_write = (rw_mode_t)-1;
  config_out->block_size_bytes = 0U;
  config_out->block_count_total = 0ULL;
  config_out->file_path = NULL;
  config_out->range_start_byte = 0;
  config_out->range_end_byte = 0;
  config_out->use_direct_io = 0;
  config_out->selection_mode = (sel_mode_t)-1;

  for (int arg_index = 1; arg_index < argc; ++arg_index) {
    char* key_value = argv[arg_index];
    char* equal_sign_ptr = strchr(key_value, '=');
    if (equal_sign_ptr == NULL) {
      print_usage("invalid param (no '=')");
      return -1;
    }

    *equal_sign_ptr = '\0';
    const char* key_str = key_value;
    const char* value_str = equal_sign_ptr + 1;

    int apply_result = apply_param(config_out, key_str, value_str);

    *equal_sign_ptr = '=';

    if (apply_result != 0) {
      return -1;
    }
  }

  if (config_out->mode_read_write == (rw_mode_t)-1 ||
      config_out->selection_mode == (sel_mode_t)-1 ||
      config_out->block_size_bytes == 0U ||
      config_out->block_count_total == 0ULL || config_out->file_path == NULL) {
    print_usage("missing required parameters");
    return -1;
  }

  return 0;
}
