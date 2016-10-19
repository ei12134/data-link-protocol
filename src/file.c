#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "file.h"

#define BUFSIZE (8*1024*1024)

int read_file_from_stdin(const char *name, struct file *f)
{
	char *buffer = malloc(sizeof(char) * 10968);
	char c;
	int size = 0;
	do {
		c = 0;
		while (size < 10968 && (c = getc(stdin)) != EOF) {
			buffer[size++] = c;
		}
	} while (c != EOF);

//	#ifdef APPLICATION_PORT_DEBUG_MODE
//	fprintf(stderr, "name=%s\n\nsize=%d\n\ndata=%s\n", name, size,
//			buffer);
//	#endif

	f->file_name = name;
	f->size = size;
	f->data = buffer;
	return 0;
}

int read_file_from_disk(const char *name, struct file *f)
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
			f->file_name = name;
			f->size = length;
			f->data = buffer;
			return 0;
		}
	}

	fprintf(stderr, "Error: file %s is NULL.\n", name);
	return -1;
}
