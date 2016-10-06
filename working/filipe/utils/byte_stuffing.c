#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "../netlink.h"
#include "byte_stuffing.h"

// TODO: substituir por constantes dos outros ficheiros.
#define ESC_C 0x7d
#define FLAG_C FLAG
#define OCT_C 0x20

/* Byte stuffing:
 * - FLAG_C -> ESC_C OCT_C^FLAG_C
 * - ESC_C  -> ESC_C OCT_C^ESC_C
 *
 * Nota: Se aparecer, por exemplo, "ESC_C OCT_C^ESC_C" na trama original,
 * este é substituído por "ESC_C OCT_C^ESC_C OCT_C^ESC_C", por isso não há
 * problemas com ambiguidades.
 */

int count_escapes(char *data, size_t sz)
{
	int count = 0;
	int i;
	for (i = 0; i < sz; i++) {
		char c = data[i];
		if (c == FLAG_C || c == ESC_C)
			++count;
	}
	return count;
}

void insert_escapes(char *data, size_t sz, int count, char* new_data)
{
	int i = 0, j = 0;
	while (i < sz) {
		char c = data[i++];
		if (c == ESC_C || c == FLAG_C) {
			new_data[j++] = ESC_C;
			new_data[j++] = OCT_C ^ c;
		} else {
			new_data[j++] = c;
		}
	}
}

/* Retorna novo vector com byte stuffing. Não esquecer de libertar a
 * memória com "free()". */
int stuff_bytes(char* data, size_t sz, char** out)
{
	if (sz == 0) {
		return 0;
	}

	int count = count_escapes(data, sz);
	/* TODO: optimização: se count==0, podemos usar o 'data' sem modificação.
	 if (count == 0) {
	 *out == NULL;
	 return 0;
	 }
	 */

	char *new_data = (char*) malloc((sz + count) * sizeof(char));
	if (new_data == NULL ) {
		fprintf(stderr, "stuff_bytes: erro na alocação de memória.");
		return -1; // error
	}
	insert_escapes(data, sz, count, new_data);
	*out = new_data;
	return count + sz;
}

/* Possível erro: último caracter é um ESC_C. */
int destuff_bytes(char *data, size_t sz)
{
	if (data[sz - 1] == ESC_C) {
		fprintf(stderr, "destuff_bytes: byte stuffing inválido: último ESC_C.");
		return -1; // error
	}

	int i;
	int j = 0;
	for (i = 0; i < sz; i++) {
		char c = data[i];
		data[j++] = (c == ESC_C) ? data[++i] ^ OCT_C : c;
	}
	return j;
}

/*
 * TESTS
 */

void print_sequence(char* name, char* s, int sz)
{
	printf("%s:", name);
	while (sz-- > 0) {
		printf(" %x", *s++);
	}
	putchar('\n');
}

void test(char* before, char *after)
{
	printf("--- test ---\n");
	print_sequence("before ", before, strlen(before));

	char* result;
	int sz = stuff_bytes(before, strlen(before), &result);
	print_sequence("result ", result, sz);
	print_sequence("correct", after, strlen(after));
	assert(strncmp(result, after, sz) == 0);

	sz = destuff_bytes(result, sz);
	print_sequence("destuff", result, sz);
	assert(strncmp(result, before, sz) == 0);

	free(result);
}

void test_1()
{
	char before[] = "abcd";
	test(before, before);
}

void test_2()
{
	char before[2];
	before[0] = ESC_C;
	before[1] = '\0';

	char after[3];
	after[0] = ESC_C;
	after[1] = ESC_C ^ OCT_C;
	after[2] = '\0';

	test(before, after);
}

void test_3()
{
	char before[2];
	before[0] = FLAG_C;
	before[1] = '\0';

	char after[3];
	after[0] = ESC_C;
	after[1] = FLAG_C ^ OCT_C;
	after[2] = '\0';

	test(before, after);
}

void test_4()
{
	char before[] = "flag: x, escape: x, ...";
	before[6] = FLAG_C;
	before[17] = ESC_C;

	char after[] = "flag: xx, escape: xx, ...";
	after[6] = ESC_C;
	after[7] = FLAG_C ^ OCT_C;
	after[18] = ESC_C;
	after[19] = ESC_C ^ OCT_C;

	test(before, after);
}

void test_5()
{
	char before[] = "abc xx abc";
	before[4] = ESC_C;
	before[5] = ESC_C ^ OCT_C;

	char after[] = "abc xxx abc";
	after[4] = ESC_C;
	after[5] = ESC_C ^ OCT_C;
	after[6] = ESC_C ^ OCT_C;

	test(before, after);
}

void run_all_byte_stuffing_tests()
{
	test_1();
	test_2();
	test_3();
	test_4();
	test_5();
}
