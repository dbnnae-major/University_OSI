#define _GNU_SOURCE
#include "io.h"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "util.h"

static const size_t ALIGN_512_BYTES = 512U;
static const size_t ALIGN_4096_BYTES = 4096U;
static const unsigned int BYTE_MASK_0xFFU = 0xFFU;

static unsigned int build_open_flags(const Config* config_in) {
  unsigned int flags = (config_in->mode_read_write == MODE_READ)
                           ? (unsigned int)O_RDONLY
                           : (unsigned int)O_WRONLY;
  if (config_in->use_direct_io != 0) {
    flags |= (unsigned int)O_DIRECT;
  }
  return flags;
}

static int open_and_stat_file(
    const char* path_cstr, unsigned int flags, int* fd_out, off_t* file_size_out
) {
  int fd_local = open(path_cstr, (int)flags);
  if (fd_local < 0) {
    perror("open");
    return 2;
  }
  struct stat file_stat_buffer;
  if (fstat(fd_local, &file_stat_buffer) != 0) {
    perror("fstat");
    (void)close(fd_local);
    return 2;
  }
  *fd_out = fd_local;
  *file_size_out = file_stat_buffer.st_size;
  return 0;
}

static off_t align_up_off_t(off_t value, off_t alignment) {
  off_t remainder = value % alignment;
  if (remainder == 0) {
    return value;
  }
  return value + (alignment - remainder);
}

static int allocate_io_buffer(
    const Config* config_in, off_t effective_start, void** io_buffer_out
) {
  void* io_buffer_local = NULL;

  if (config_in->use_direct_io != 0) {
    if ((config_in->block_size_bytes % ALIGN_512_BYTES) != 0U) {
      (void)fprintf(
          stderr, "error: with direct=on, block_size must be multiple of 512\n"
      );
      return 2;
    }
    if ((effective_start % (off_t)ALIGN_512_BYTES) != 0) {
      (void)fprintf(
          stderr, "error: with direct=on, range start must be 512-aligned\n"
      );
      return 2;
    }
    int rc_alloc = allocate_aligned(
        &io_buffer_local, ALIGN_4096_BYTES, config_in->block_size_bytes
    );
    if (rc_alloc != 0) {
      (void)fprintf(stderr, "posix_memalign failed\n");
      return 2;
    }
  } else {
    io_buffer_local = malloc(config_in->block_size_bytes);
    if (io_buffer_local == NULL) {
      (void)fprintf(stderr, "malloc failed\n");
      return 2;
    }
  }

  *io_buffer_out = io_buffer_local;
  return 0;
}

static void fill_write_pattern(void* io_buffer, size_t block_size_bytes) {
  unsigned char* bytes = (unsigned char*)io_buffer;
  for (size_t byte_index = 0U; byte_index < block_size_bytes; ++byte_index) {
    bytes[byte_index] = (unsigned char)(byte_index & BYTE_MASK_0xFFU);
  }
}

static void adjust_range_for_mode(
    const Config* config_in,
    off_t file_size_bytes,
    off_t* effective_start_inout,
    off_t* effective_end_inout,
    off_t* placement_limit_end_out
) {
  off_t effective_start = *effective_start_inout;
  off_t effective_end = *effective_end_inout;

  if (config_in->mode_read_write == MODE_READ) {
    if (effective_end > file_size_bytes) {
      effective_end = file_size_bytes;
    }
    *placement_limit_end_out = effective_end;
  } else {
    off_t proposed_end =
        (config_in->range_start_byte == 0 && config_in->range_end_byte == 0)
            ? file_size_bytes
            : effective_end;
    if (proposed_end < effective_start) {
      proposed_end = effective_start;
    }
    *placement_limit_end_out = proposed_end;
  }

  *effective_start_inout = effective_start;
  *effective_end_inout = effective_end;
}

static long long compute_available_slots(
    off_t aligned_base_offset,
    off_t placement_limit_end,
    size_t block_size_bytes
) {
  if (placement_limit_end > aligned_base_offset && block_size_bytes > 0U) {
    return (long long)((placement_limit_end - aligned_base_offset) /
                       (off_t)block_size_bytes);
  }
  return 0LL;
}

static off_t pick_target_offset(
    const Config* config_in,
    off_t aligned_base_offset,
    long long available_slots,
    unsigned long long block_index
) {
  if (config_in->selection_mode == SEL_SEQUENCE) {
    long long slot_index =
        (long long)(block_index % (unsigned long long)available_slots);
    return aligned_base_offset +
           (off_t)slot_index * (off_t)config_in->block_size_bytes;
  }
  unsigned long long slot_index_random =
      get_uniform_random_below((unsigned long long)available_slots);
  return aligned_base_offset +
         (off_t)slot_index_random * (off_t)config_in->block_size_bytes;
}

static int perform_single_io(
    const Config* config_in,
    int file_descriptor,
    void* io_buffer,
    off_t target_offset
) {
  ssize_t io_result_size = 0;

  if (config_in->mode_read_write == MODE_READ) {
    io_result_size = pread(
        file_descriptor, io_buffer, config_in->block_size_bytes, target_offset
    );
    if (io_result_size < 0) {
      perror("pread");
      return 2;
    }
    if ((size_t)io_result_size != config_in->block_size_bytes) {
      (void)fprintf(
          stderr,
          "short read at offset %" PRIu64 " (got %zd)\n",
          (uint64_t)target_offset,
          io_result_size
      );
      return 2;
    }
    volatile unsigned char prevent_optimization =
        ((unsigned char*)io_buffer)[0];
    (void)prevent_optimization;  // хак нахуй:>
    return 0;
  }
  io_result_size = pwrite(
      file_descriptor, io_buffer, config_in->block_size_bytes, target_offset
  );
  if (io_result_size < 0) {
    perror("pwrite");
    return 2;
  }
  if ((size_t)io_result_size != config_in->block_size_bytes) {
    (void)fprintf(
        stderr,
        "short write at offset %" PRIu64 " (wrote %zd)\n",
        (uint64_t)target_offset,
        io_result_size
    );
    return 2;
  }
  return 0;
}

int run_io(const Config* config_in) {
  unsigned int open_flags = build_open_flags(config_in);

  int file_descriptor = -1;
  off_t file_size_bytes = 0;
  {
    int open_rc = open_and_stat_file(
        config_in->file_path, open_flags, &file_descriptor, &file_size_bytes
    );
    if (open_rc != 0) {
      return open_rc;
    }
  }

  off_t effective_start = config_in->range_start_byte;
  off_t effective_end =
      (config_in->range_start_byte == 0 && config_in->range_end_byte == 0)
          ? file_size_bytes
          : config_in->range_end_byte;

  if (effective_end < effective_start) {
    (void)fprintf(stderr, "error: empty range\n");
    (void)close(file_descriptor);
    return 2;
  }

  void* io_buffer = NULL;
  {
    int buf_rc = allocate_io_buffer(config_in, effective_start, &io_buffer);
    if (buf_rc != 0) {
      (void)close(file_descriptor);
      return buf_rc;
    }
  }

  if (config_in->mode_read_write == MODE_WRITE) {
    fill_write_pattern(io_buffer, config_in->block_size_bytes);
  }

  off_t placement_limit_end = 0;
  adjust_range_for_mode(
      config_in,
      file_size_bytes,
      &effective_start,
      &effective_end,
      &placement_limit_end
  );

  off_t aligned_base_offset =
      align_up_off_t(effective_start, (off_t)config_in->block_size_bytes);

  if (config_in->use_direct_io != 0 &&
      (aligned_base_offset % (off_t)ALIGN_512_BYTES) != 0) {
    (void)fprintf(
        stderr, "error: with direct=on, starting offset must be 512-aligned\n"
    );
    free(io_buffer);
    (void)close(file_descriptor);
    return 2;
  }

  long long available_slots = compute_available_slots(
      aligned_base_offset, placement_limit_end, config_in->block_size_bytes
  );
  if (available_slots <= 0LL) {
    (void)fprintf(stderr, "error: no space for at least one block in range\n");
    free(io_buffer);
    (void)close(file_descriptor);
    return 2;
  }

  for (unsigned long long block_index = 0ULL;
       block_index < config_in->block_count_total;
       ++block_index) {
    off_t target_offset = pick_target_offset(
        config_in, aligned_base_offset, available_slots, block_index
    );
    int io_rc =
        perform_single_io(config_in, file_descriptor, io_buffer, target_offset);
    if (io_rc != 0) {
      free(io_buffer);
      (void)close(file_descriptor);
      return io_rc;
    }
  }

  free(io_buffer);
  if (close(file_descriptor) != 0) {
    perror("close");
    return 2;
  }
  return 0;
}
