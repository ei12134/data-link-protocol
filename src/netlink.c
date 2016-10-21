#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "packets.h"
#include "file.h"

#define TRUE 1
#define FALSE 0

struct file file;

// Print how the arguments must be
void print_help(char **argv)
{
	fprintf(stderr, "Usage: %s [OPTION] <serial port>\n", argv[0]);
	fprintf(stderr, "\n Program options:\n");
	fprintf(stderr, "  -t         transmit data over the serial port\n");
}

// Verifies serial port argument
int parse_serial_port_arg(int index, char **argv)
{
	if ((strcmp("/dev/ttyS0", argv[index]) != 0)
			&& (strcmp("/dev/ttyS1", argv[index]) != 0)
			&& (strcmp("/dev/ttyS4", argv[index]) != 0))
		return -3;

	return index;
}

// Verifies arguments
int parse_args(int argc, char **argv, int *is_transmitter)
{
	if (argc < 2)
		return -1;

	if (argc == 2)
		return parse_serial_port_arg(1, argv);

	if (argc == 3) {
		if ((strcmp("-t", argv[1]) != 0))
			return -2;
		else
			*is_transmitter = 1;

		if (read_file_from_stdin(argv[2], &file) < 0) {
			return -1;
		}
		return parse_serial_port_arg(2, argv);
	}

	if (argc == 4) {
		if ((strcmp("-t", argv[1]) != 0))
			return -2;
		else {
			*is_transmitter = 1;
		}

		if (read_file_from_disk(argv[2], &file) < 0) {
			return -1;
		}
		return parse_serial_port_arg(3, argv);
	} else
		return -1;
}

int main(int argc, char **argv)
{
	// Verifies arguments
	int i = -1;
	int is_transmitter = 0;
	if ((i = parse_args(argc, argv, &is_transmitter)) < 0) {
		print_help(argv);
		exit(1);
	}

	if (is_transmitter) {
		fprintf(stderr, "netlink: transmitting...\n");
		return send_file(argv[i], &file);
	} else {
		fprintf(stderr, "netlink: receiving...\n");
		return receive_file(argv[i], 5);
	}
}
