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
int max_retries = 1;

void help(char **argv)
{
	fprintf(stderr, "Usage: %s [OPTIONS] <serial port>\n", argv[0]);
	fprintf(stderr, "\n Program options:\n");
	fprintf(stderr, "  -t <FILEPATH>\t\ttransmit file over the serial port\n");
	fprintf(stderr, "  -i\t\t\ttransmit data read from stdin\n");
	fprintf(stderr, "  -b <BAUDRATE>\t\tbaudrate of the serial port\n");
	fprintf(stderr,
			"  -p <DATASIZE>\tmaximum bytes of data transfered in each frame\n");
	fprintf(stderr, "  -r <RETRY>\t\tnumber of retry attempts\n");
}

int parse_serial_port_arg(int index, char **argv)
{
	if ((strcmp("/dev/ttyS0", argv[index]) != 0)
			&& (strcmp("/dev/ttyS1", argv[index]) != 0)
			&& (strcmp("/dev/ttyS4", argv[index]) != 0)) {
		fprintf(stderr, "Error: bad serial port value\n");
		return -1;
	}

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
	fprintf(stderr, "Error: bad serial port baudrate value\n");
	fprintf(stderr,
			"Valid baudrates: B110, B134, B150, B200, B300, B600, B1200, B1800, B2400, B4800, B9600, B19200, B38400\n");
	return -1;
}

void parse_max_packet_size(int packet_size_index, char **argv)
{
	int val = atoi(argv[packet_size_index]);
	if (val > FRAME_SIZE || val < 0)
		max_data_transfer = FRAME_SIZE;
	else
		max_data_transfer = val;

#ifdef NETLINK_DEBUG_MODE
	fprintf(stderr,"\nparse_max_packet_size:\n");
	fprintf(stderr,"  max_packet_size=%d\n", max_data_transfer);
#endif
}

void parse_max_retries(int packet_size_index, char **argv)
{
	int val = atoi(argv[packet_size_index]);
	if (val <= 0)
		max_retries = 1;
	else
		max_retries = 1 + val;

#ifdef NETLINK_DEBUG_MODE
	fprintf(stderr,"\nmax_retries:\n");
	fprintf(stderr,"  max_retries=%d\n", max_retries);
#endif
}

int parse_flags(int* t_index, int* i_index, int* b_index, int* p_index,
		int* r_index, int argc, char **argv)
{
	for (size_t i = 0; i < (argc - 1); i++) {
		if ((strcmp("-t", argv[i]) == 0)) {
			*t_index = i;
		} else if ((strcmp("-i", argv[i]) == 0)) {
			*i_index = i;
		} else if ((strcmp("-b", argv[i]) == 0)) {
			*b_index = i;
		} else if ((strcmp("-p", argv[i]) == 0)) {
			*p_index = i;
		} else if ((strcmp("-r", argv[1]) == 0)) {
			*r_index = i;
		} else if ((argv[i][0] == '-')) {
			return -1;
		}
	}
#ifdef NETLINK_DEBUG_MODE
	fprintf(stderr,"\nparse_flags(): flag indexes\n");
	fprintf(stderr,"  -t=%d\n  -i=%d\n  -b=%d\n  -p=%d\n  -r=%d\n", *t_index, *i_index, *b_index,
			*p_index, *r_index);
#endif
	return 0;
}

int parse_args(int argc, char **argv, int *is_transmitter)
{

#ifdef NETLINK_DEBUG_MODE
	fprintf(stderr,"\nparse_args(): received arguments\n");
	fprintf(stderr,"  argc=%d\n  argv=%s\n", argc, *argv);
#endif

	if (argc < 2) {
		return -1;
	}

	if (argc == 2)
		return parse_serial_port_arg(1, argv);

	int t_index = -1, i_index = -1, b_index = -1, p_index = -1, r_index = -1;

	if (parse_flags(&t_index, &i_index, &b_index, &p_index, &r_index, argc,
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

	if (p_index > 0 && p_index < argc - 1) {
		parse_max_packet_size(p_index + 1, argv);
	}

	if (r_index > 0 && r_index < argc - 1) {
		parse_max_retries(r_index + 1, argv);
	}

	return parse_serial_port_arg(argc - 1, argv);
}

int main(int argc, char **argv)
{
	int port_index = -1;
	int is_transmitter = 0;

	if ((port_index = parse_args(argc, argv, &is_transmitter)) < 0) {
		help(argv);
		exit(EXIT_FAILURE);
	}

	if (is_transmitter) {
		fprintf(stderr, "transmitting %s\n", file_to_send.name);
		return send_file(argv[port_index], &file_to_send, max_retries);
	} else {
		fprintf(stderr, "receiving file\n");
#ifdef NETLINK_DEBUG_MODE
		fprintf(stderr, "\tserial_port_baudrate:%d\n", serial_port_baudrate);
		fprintf(stderr, "\tis_transmitter:%d\n", is_transmitter);
#endif
		return receive_file(argv[port_index], max_retries);
	}
}
