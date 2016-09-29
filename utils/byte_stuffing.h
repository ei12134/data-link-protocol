#ifndef BYTE_STUFFING_H
#define BYTE_STUFFING_H

#include <stdio.h>

int stuff_bytes(char *data, size_t sz, char **out);
int destuff_bytes(char *data, size_t sz);
void run_all_byte_stuffing_tests();

#endif /* BYTE_STUFFING_H */
