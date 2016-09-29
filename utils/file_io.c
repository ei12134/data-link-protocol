#include "file_io.h"

FILE *openFile(char *filename, const char *type) {
  FILE *f;
  f = fopen(filename, type);

  if (f == NULL)
    printf("Error creating the file: %s./n", filename);

  return f;
}

int closeFile(FILE *file) { return fclose(file); }

int writeFile(char *string, FILE *f, int length) {
  if (f != 0)
    return fwrite(string, sizeof(char), length, f);

  return -1;
}

int readFile(char *string, int nbytes, FILE *f) {
  return fread(string, 1, nbytes, f);
}