#pragma once

#include <stddef.h>

unsigned long long get_uniform_random_below(unsigned long long upper_exclusive);
int allocate_aligned(void **pointer_out, size_t alignment_bytes, size_t allocation_size_bytes);
