#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WORD_SIZE 8
#define HASH_SIZE 10007

typedef struct Row {
  int id;
  char word[WORD_SIZE + 1];
} Row;

typedef struct Node {
  Row row;
  struct Node* next;
} Node;

static unsigned hash_int(int id) {
  return (unsigned)id % HASH_SIZE;
}

static Row* read_table(const char* path, int* out_count) {
  FILE* file = fopen(path, "r");
  if (!file) {
    (void)fprintf(stderr, "Cannot open %s\n", path);
    return NULL;
  }

  if (fscanf(file, "%d", out_count) != 1) {
    (void)fclose(file);
    (void)fprintf(stderr, "Invalid format\n");
    return NULL;
  }

  Row* rows = (Row*)malloc((size_t)*out_count * sizeof(Row));
  if (!rows) {
    (void)fclose(file);
    (void)fprintf(stderr, "malloc failed\n");
    return NULL;
  }

  for (int i = 0; i < *out_count; ++i) {
    if (fscanf(file, "%d %8s", &rows[i].id, rows[i].word) != 2) {
      (void)fprintf(stderr, "Invalid row in %s\n", path);
      free(rows);
      (void)fclose(file);
      return NULL;
    }
  }

  (void)fclose(file);
  return rows;
}

int main(int argc, char** argv) {
  if (argc != 4) {
    (void)fprintf(stderr, "Usage: %s <fileA> <fileB> <output>\n", argv[0]);
    return 1;
  }

  int countA = 0;
  Row* tableA = read_table(argv[1], &countA);
  if (!tableA) {
    return 1;
  }

  int countB = 0;
  Row* tableB = read_table(argv[2], &countB);
  if (!tableB) {
    free(tableA);
    return 1;
  }

  Node* hash[HASH_SIZE];
  for (int i = 0; i < HASH_SIZE; ++i) {
    hash[i] = NULL;
  }

  for (int i = 0; i < countA; ++i) {
    unsigned h = hash_int(tableA[i].id);
    Node* n = (Node*)malloc(sizeof(Node));
    n->row = tableA[i];
    n->next = hash[h];
    hash[h] = n;
  }

  FILE* out = fopen(argv[3], "w");
  if (!out) {
    (void)fprintf(stderr, "Cannot open output file\n");
    free(tableA);
    free(tableB);
    return 1;
  }

  for (int i = 0; i < countB; ++i) {
    unsigned h = hash_int(tableB[i].id);
    Node* cur = hash[h];

    while (cur) {
      if (cur->row.id == tableB[i].id) {
        (void
        )fprintf(out, "%d %s %s\n", cur->row.id, cur->row.word, tableB[i].word);
      }
      cur = cur->next;
    }
  }

  (void)fclose(out);

  for (int i = 0; i < HASH_SIZE; ++i) {
    Node* cur = hash[i];
    while (cur) {
      Node* next = cur->next;
      free(cur);
      cur = next;
    }
  }

  free(tableA);
  free(tableB);

  return 0;
}
