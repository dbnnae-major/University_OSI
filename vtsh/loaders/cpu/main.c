#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define base 10

static void fill_matrix(double* matrix, int size) {
  for (int i = 0; i < size * size; ++i) {
    matrix[i] = (double)rand() / (double)RAND_MAX;
  }
}

static void mat_mul(
    const double* matrix_a, const double* matrix_b, double* matrix_c, int n
) {
  for (int i = 0; i < n; ++i) {
    for (int j = 0; j < n; ++j) {
      double sum = 0.0;
      for (int k = 0; k < n; ++k) {
        sum += matrix_a[i * n + k] * matrix_b[k * n + j];
      }
      matrix_c[i * n + j] = sum;
    }
  }
}

int main(int argc, char** argv) {
  if (argc != 3) {
    (void)fprintf(stderr, "Usage: %s <matrix_size> <repeat_count>\n", argv[0]);
    return 1;
  }

  int size = (int)strtoul(argv[1], NULL, base);
  int repeat = (int)strtoul(argv[2], NULL, base);

  if (size <= 0 || repeat <= 0) {
    (void)fprintf(stderr, "matrix_size and repeat_count must be > 0\n");
    return 1;
  }

  size_t total_bytes = (size_t)size * (size_t)size * sizeof(double);

  double* matrix_a = (double*)malloc(total_bytes);
  double* matrix_b = (double*)malloc(total_bytes);
  double* matrix_c = (double*)malloc(total_bytes);

  if (!matrix_a || !matrix_b || !matrix_c) {
    (void)fprintf(stderr, "malloc error\n");
    free(matrix_a);
    free(matrix_b);
    free(matrix_c);
    return 1;
  }

  srand((unsigned)clock());
  fill_matrix(matrix_a, size);
  fill_matrix(matrix_b, size);

  for (int cnt = 0; cnt < repeat; ++cnt) {
    mat_mul(matrix_a, matrix_b, matrix_c, size);
  }

  double checksum = 0.0;
  for (int i = 0; i < size * size; ++i) {
    checksum += matrix_c[i];
  }
  printf("Checksum: %.6f\n", checksum);

  free(matrix_a);
  free(matrix_b);
  free(matrix_c);

  return 0;
}
