#pragma once

#include <stddef.h>
#include <sys/types.h>

typedef enum { MODE_READ, MODE_WRITE } rw_mode_t;
typedef enum { SEL_SEQUENCE, SEL_RANDOM } sel_mode_t;

typedef struct {
    rw_mode_t mode_read_write;
    size_t block_size_bytes;
    unsigned long long block_count_total;
    const char *file_path;
    off_t range_start_byte;
    off_t range_end_byte;
    int use_direct_io;
    sel_mode_t selection_mode;
} Config;

int parse_args(int argc, char **argv, Config *config_out);
void print_usage(const char *message);
