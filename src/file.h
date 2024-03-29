#ifndef FILE_H_
#define FILE_H_

#include "byte.h"

struct file {
	const char* name;
	size_t size;
	char* data;
};

int read_file_from_stdin(struct file *f);
int read_file_from_disk(char *name, struct file *f);

#endif // FILE_H_
