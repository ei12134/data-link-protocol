#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "../link/serial_port.h"
#include "state_machine.h"
#include "transmitter.h"

int alarm_flag = TRUE;

void answer_alarm()
{
	alarm_flag = TRUE;
}

int transmitter(char *serial_port)
{
	// set a SIGALARM signal handler
	(void) signal(SIGALRM, answer_alarm);

	// initialize serial port session
	int fd = llopen(serial_port, 30, TRANSMITTER);

	// send SET

	// end serial port session
	llclose(fd);

	return 0;
}
