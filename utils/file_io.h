#ifndef FILE_IO_H
#define FILE_IO_H

#include <stdio.h>

FILE* openFile(char * filename, const char * type)																																										char pras);
int closeFile(FILE *file);
int writeFile(char *string, FILE *f, int length);
int readFile(char *string, int nbytes, FILE *f);

#endif /* FILE_IO_H */
