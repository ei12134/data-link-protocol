#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <libgen.h>
#include <limits.h>
#include "file.h"

int read_file_from_stdin(struct file *f)
{
	char *buffer;
	if ((buffer = malloc(sizeof(char) * INT_MAX)) == NULL ) {
		perror("read_file_from_stdin() buffer malloc error");
		return -1;
	}

	size_t size = 0;

	if ((size = fread(buffer, sizeof(char), INT_MAX, stdin)) < 0) {
		fprintf(stderr, "ERROR: reading from the stdin.\n");
		return -1;
	}

#ifdef APPLICATION_LAYER_DEBUG_MODE
	fprintf(stderr, "read_file_from_stdin()\n\tname=%s\n\tsize=%zu\n\tdata=%s\n", "stdin.out", size,
			buffer);
#endif

	f->name = "output";
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
