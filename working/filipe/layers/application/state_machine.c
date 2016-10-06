#include <stdio.h>
#include <string.h>
#include "state_machine.h"
#include "../link/serial_port.h"

int receive_set_frame(int fd)
{
	char received[BUFSIZE] = { 0 };
	int size = 0;

	if ((size = llread(fd, received)) < 0) {
		printf("llread error: %d\n", size);
	}

	printf("%s:%d\n", received, size);
	return 0;
}
