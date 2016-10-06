#include <stdio.h>
#include <stdlib.h>

/*
gcc -o test test.c
./test "cenas.txt"
*/
char* readFileBytes(const char *name)
{
	FILE *file = fopen(name, "r");
	if (file != NULL)
	{
		fseek(file, 0L, SEEK_END);
		long length = ftell(file);
		char *buffer = malloc(length);
		if(buffer != NULL)
		{			
			fseek(file, 0, SEEK_SET);
			fread(buffer, 1, length, file);
			//printf("O conteúdo do ficheiro é: %s\n", buffer);
			fclose(file);
			return buffer;
		}
	}
	printf("File is NULL.\n");
	return 0;
}

int main(int argc, char *argv[])
{	
	printf("O Ficheiro lido é: %s \n", argv[1]);
	char* cenas;
	cenas = readFileBytes(argv[1]);

	printf("O conteúdo do ficheiro é: %s", cenas);
	
	return 0 ;
}
