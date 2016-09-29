#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "receiver.h"
#include "state_machine.h"
#include "../link/serial_port.h"

int receiver(char *serial_port)
{
	int fd = -1;

	// initialize serial port session
	if ((fd = llopen(serial_port, 0, RECEIVER) < 0))
		return -1;

	// default receiver state-machine
	// receiver_state_machine(fd);

	// SET receiver state machine
	if (receive_set(fd) < 0)
		return -1;

	// end serial port session
	if (llclose(fd) < 0)
		return -1;

	return 0;
}
