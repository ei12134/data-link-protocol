#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include "file.h"

int read_file_from_stdin(struct file *f)
{
	char *buffer = malloc(sizeof(char) * sizeof(size_t));
	char c = 0;
	int size = 0;

	while (size < sizeof(size_t) && (c = fgetc(stdin)) != EOF) {
		buffer[size++] = c;
	}

#ifdef APPLICATION_PORT_DEBUG_MODE
	fprintf(stderr, "read_file_from_stdin()\nname=%s\n\tsize=%d\n\tndata=%s\n", name, size,
			buffer);
#endif

	f->name = "stdin.out";
	f->size = size;
	f->data = buffer;
	return 0;
}

int read_file_from_disk(char *name, struct file *f)
{
	size_t length;
	FILE *file = fopen(name, "r");

	if (file != NULL ) {
		fseek(file, 0L, SEEK_END);
		length = ftell(file);
		char *buffer = malloc(sizeof(char) * length);
		if (buffer != NULL ) {
			fseek(file, 0, SEEK_SET);
			fread(buffer, 1, length, file);
			fclose(file);
			f->name = basename(name);
			f->size = length;
			f->data = buffer;
			return 0;
		}
	}

	fprintf(stderr, "Error: file %s is NULL.\n", name);
	return -1;
}
