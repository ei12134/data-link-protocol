#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "packets.h"
#include "file.h"

struct file file_to_send;
size_t real_file_bytes = 0;
size_t received_file_bytes = 0;
size_t lost_packets = 0;
size_t duplicated_packets = 0;

void help(char **argv)
{
	fprintf(stderr, "Usage: %s [OPTIONS] <serial port>\n", argv[0]);
	fprintf(stderr, "\n Program options:\n");
	fprintf(stderr, "  -t <FILEPATH>\t\ttransmit data over the serial port\n");
	fprintf(stderr, "  -b <BAUDRATE>\t\tbaudrate of the serial port\n");
	fprintf(stderr, "  -i <FRAMESIZE>\tmaximum unstuffed frame size\n");
	fprintf(stderr, "  -r <RETRY>\t\tnumber of retry attempts\n");
}

int parse_serial_port_arg(int index, char **argv)
{
	if ((strcmp("/dev/ttyS0", argv[index]) != 0)
			&& (strcmp("/dev/ttyS1", argv[index]) != 0)
			&& (strcmp("/dev/ttyS4", argv[index]) != 0))
		return -3;

	return index;
}

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

		if (read_file_from_stdin(&file_to_send) < 0) {
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

		if (read_file_from_disk(argv[2], &file_to_send) < 0) {
			return -1;
		}
		return parse_serial_port_arg(3, argv);
	} else
		return -1;
}

void receiver_stats()
{
	fprintf(stdout, "Receiver statistics\n");
	fprintf(stdout, "\treceived file bytes/file bytes: %zu/%zu\n",
			received_file_bytes, real_file_bytes);
	fprintf(stdout, "\tlost packets: %zu\n", lost_packets);
	fprintf(stdout, "\tduplicated packets: %zu\n", duplicated_packets);
}

int main(int argc, char **argv)
{
	int port_index = -1;
	int is_transmitter = 0;
	int exit_code = 0;

	if ((port_index = parse_args(argc, argv, &is_transmitter)) < 0) {
		help(argv);
		exit(EXIT_FAILURE);
	}

	if (is_transmitter) {
		fprintf(stderr, "netlink: transmitting %s\n", file_to_send.name);
		exit_code = send_file(argv[port_index], &file_to_send, 5);
	} else {
		fprintf(stderr, "netlink: receiving file\n");
		exit_code = receive_file(argv[port_index], 5);
		receiver_stats();
	}

	exit(exit_code);
}
