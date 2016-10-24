#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#include "packets.h"
#include "file.h"
#include "netlink.h"
#include "serial_port.h"

struct file file_to_send;

size_t real_file_bytes = 0;
size_t received_file_bytes = 0;
size_t lost_packets = 0;
size_t duplicated_packets = 0;

void help(char **argv)
{
	fprintf(stderr, "Usage: %s [OPTIONS] <serial port>\n", argv[0]);
	fprintf(stderr, "\n Program options:\n");
	fprintf(stderr, "  -t <FILEPATH>\t\ttransmit file over the serial port\n");
	fprintf(stderr, "  -i\t\t\ttransmit data read from stdin\n");
	fprintf(stderr, "  -b <BAUDRATE>\t\tbaudrate of the serial port\n");
	fprintf(stderr, "  -f <FRAMESIZE>\tmaximum unstuffed frame size\n");
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

int parse_baudrate_arg(int baurdate_index, char **argv)
{
	if (strcmp("B50", argv[baurdate_index]) == 0) {
		serial_port_baudrate = B50;
		return 0;
	} else if (strcmp("B75", argv[baurdate_index]) == 0) {
		serial_port_baudrate = B75;
		return 0;
	} else if (strcmp("B110", argv[baurdate_index]) == 0) {
		serial_port_baudrate = B110;
		return 0;
	} else if (strcmp("B134", argv[baurdate_index]) == 0) {
		serial_port_baudrate = B134;
		return 0;
	} else if (strcmp("B150", argv[baurdate_index]) == 0) {
		serial_port_baudrate = B150;
		return 0;
	} else if (strcmp("B200", argv[baurdate_index]) == 0) {
		serial_port_baudrate = B200;
		return 0;
	} else if (strcmp("B300", argv[baurdate_index]) == 0) {
		serial_port_baudrate = B300;
		return 0;
	} else if (strcmp("B600", argv[baurdate_index]) == 0) {
		serial_port_baudrate = B600;
		return 0;
	} else if (strcmp("B1200", argv[baurdate_index]) == 0) {
		serial_port_baudrate = B1200;
		return 0;
	} else if (strcmp("B1800", argv[baurdate_index]) == 0) {
		serial_port_baudrate = B1800;
		return 0;
	} else if (strcmp("B2400", argv[baurdate_index]) == 0) {
		serial_port_baudrate = B2400;
		return 0;
	} else if (strcmp("B4800", argv[baurdate_index]) == 0) {
		serial_port_baudrate = B4800;
		return 0;
	} else if (strcmp("B9600", argv[baurdate_index]) == 0) {
		serial_port_baudrate = B9600;
		return 0;
	} else if (strcmp("B19200", argv[baurdate_index]) == 0) {
		serial_port_baudrate = B19200;
		return 0;
	} else if (strcmp("B38400", argv[baurdate_index]) == 0) {
		serial_port_baudrate = B38400;
		return 0;
	}
	return -1;
}

int parse_flags(int* t_index, int* i_index, int* b_index, int* f_index,
		int* r_index, int argc, char **argv)
{
	for (size_t i = 0; i < (argc - 1); i++) {
		if ((strcmp("-t", argv[i]) == 0)) {
			*t_index = i;
		} else if ((strcmp("-i", argv[i]) == 0)) {
			*i_index = i;
		} else if ((strcmp("-b", argv[i]) == 0)) {
			*b_index = i;
		} else if ((strcmp("-f", argv[i]) == 0)) {
			*f_index = i;
		} else if ((strcmp("-r", argv[1]) == 0)) {
			*r_index = i;
		} else if ((argv[i][0] == '-')) {
			return -1;
		}
	}
#ifdef NETLINK_DEBUG_MODE
	fprintf(stderr,"parse_flags()\n");
	fprintf(stderr,"\t-t:%d   -i:%d   -b:%d   -f:%d   -r:%d\n", *t_index, *i_index, *b_index,
			*f_index, *r_index);
#endif
	return 0;
}

int parse_args(int argc, char **argv, int *is_transmitter)
{
	if (argc < 2) {
		return -1;
	}

	if (argc == 2)
		return parse_serial_port_arg(1, argv);

	int t_index = -1, i_index = -1, b_index = -1, f_index = -1, r_index = -1;

	if (parse_flags(&t_index, &i_index, &b_index, &f_index, &r_index, argc,
			argv)) {
		fprintf(stderr, "Error: bad flag parameter\n");
		return -1;
	}

	if (t_index > 0 && t_index < argc - 1) {
		if (read_file_from_disk(argv[t_index + 1], &file_to_send) < 0) {
			return -1;
		}
		*is_transmitter = 1;
	} else {
		if (i_index > 0 && i_index < argc - 1) {
			if (read_file_from_stdin(&file_to_send) < 0) {
				return -1;
			}
			*is_transmitter = 1;
		}
	}

	if (b_index > 0 && b_index < argc - 1) {
		if (parse_baudrate_arg(b_index + 1, argv) != 0) {
			return -1;
		}
	}

	return parse_serial_port_arg(argc - 1, argv);
}

void receiver_stats()
{
	fprintf(stdout, "Receiver statistics\n");
	fprintf(stdout, "\treceived file bytes/file bytes:%zu/%zu\n",
			received_file_bytes, real_file_bytes);
	fprintf(stdout, "\tlost packets:%zu\n", lost_packets);
	fprintf(stdout, "\tduplicated packets:%zu\n", duplicated_packets);
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
		fprintf(stderr, "transmitting %s\n", file_to_send.name);
		exit_code = send_file(argv[port_index], &file_to_send, 5);
	} else {
		fprintf(stderr, "receiving file\n");
#ifdef NETLINK_DEBUG_MODE
		fprintf(stderr, "\tserial_port_baudrate:%d\n", serial_port_baudrate);
		fprintf(stderr, "\tis_transmitter:%d\n", is_transmitter);
#endif
		exit_code = receive_file(argv[port_index], 5);
		receiver_stats();
	}

	exit(exit_code);
}
