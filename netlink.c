#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "netlink.h"
#include "layers/application/transmitter.h"
#include "layers/application/receiver.h"

int mode = RECEIVER;

void usage(char **argv)
{
	printf("Usage: %s [OPTION] <serial port>\n", argv[0]);
	printf("\nProgram mode:\n");
	printf("  -t\t\t\ttransmit data over the serial port\n");
}

int parse_serial_port_arg(int index, char **argv)
{
	if ((strcmp("/dev/ttyS0", argv[index]) != 0)
			&& (strcmp("/dev/ttyS1", argv[index]) != 0)
			&& (strcmp("/dev/ttyS2", argv[index]) != 0)
			&& (strcmp("/dev/ttyS3", argv[index]) != 0)) {
		return -1;
	}
	return index;
}

int parse_args(int argc, char **argv)
{
	if (argc < 2)
		return -1;

	if (argc == 2)
		return parse_serial_port_arg(1, argv);

	if (argc == 3) {
		if ((strcmp("-t", argv[1]) != 0))
			return -2;
		else
			mode = TRANSMITTER;
		return parse_serial_port_arg(2, argv);
	} else
		return -1;
}

int main(int argc, char **argv)
{
	// parse arguments
	int i = -1;
	if ((i = parse_args(argc, argv)) < 0) {
		usage(argv);
		exit(EXIT_FAILURE);
	}

	// set program execution mode
	switch (mode) {
	case TRANSMITTER:
		if (transmitter(argv[i]) != 0) {
			fprintf(stderr, "Transmitter aplication finished with errors\n");
			exit(EXIT_FAILURE);
		}
		break;

	case RECEIVER:
		if (receiver(argv[i]) != 0) {
			fprintf(stderr, "Receiver aplication finished with errors\n");
			exit(EXIT_FAILURE);
		}
		break;
	}

	return 0;
}
