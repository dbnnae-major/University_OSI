#include "util.h"

#include <stdint.h>
#include <stdlib.h>

static const unsigned int kRandChunkBits = 15U;
static const unsigned int kRandMixIterations = 5U;

unsigned long long get_uniform_random_below(unsigned long long upper_exclusive
) {
  unsigned long long mixed_random = 0ULL;
  for (unsigned int mix_index = 0U; mix_index < kRandMixIterations;
       ++mix_index) {
    mixed_random = (mixed_random << kRandChunkBits) ^
                   (unsigned long long)(unsigned int)rand();
  }
  if (upper_exclusive == 0ULL) {
    return 0ULL;
  }
  return mixed_random % upper_exclusive;
}

int allocate_aligned(
    void** pointer_out, size_t alignment_bytes, size_t allocation_size_bytes
) {
  return posix_memalign(pointer_out, alignment_bytes, allocation_size_bytes);
}
