#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "serial_port.h"
#include "../../netlink.h"
#include "../../utils/byte_stuffing.h"

struct termios oldtio;

char set[5] = { FLAG, A, C_SET, A ^ C_SET, FLAG };
char ua[5] = { FLAG, A, C_UA, A ^ C_UA, FLAG };
char disc[5] = { FLAG, A, C_DISC, A ^ C_DISC, FLAG };

void build_header(char *msg, char control_field)
{
	msg[0] = A;
	msg[1] = control_field;
	msg[2] = A ^ control_field;
}

char compute_bcc(char *msg, int length)
{
	char bcc = msg[0];
	int i;
	for (i = 1; i < length; i++)
		bcc ^= msg[i];

	return bcc;
}

char *frame_type(const char *packet)
{
	char *type;

	if ((strncmp(packet, set, 5) == 0))
		type = "SET";
	else if ((strncmp(packet, ua, 5) == 0))
		type = "UA";
	else if ((strncmp(packet, disc, 5) == 0))
		type = "DISC";
	else
		type = "Unknown packet type";

	return type;
}

char *packet_content(const char *packet, const int size)
{
	const char *hex = "0123456789ABCDEF";
	char *content = (char *) malloc(sizeof(char) * (3 * size));
	char *pout = content;
	const char *pin = packet;

	int i;

	if (pout) {

		for (i = 0; i < size - 1; i++) {
			*pout++ = hex[(*pin >> 4) & 0xF];
			*pout++ = hex[(*pin++) & 0xF];
			*pout++ = ':';
		}
		*pout++ = hex[(*pin >> 4) & 0xF];
		*pout++ = hex[(*pin++) & 0xF];
		*pout = 0;
	}

	return content;
}

int llread(int fd, char *buffer)
{
	int size = 0;
	int res = 0;
	char c;

	while (TRUE) {
		res = read(fd, &c, 1);
		if (res > 0) {
			buffer[size] = c;
			size++;
		} else
			return -1;
	}

	return size;
}

void llwrite(int fd, char *buffer, int length)
{
	int res = write(fd, buffer, length);

	printf("Sent packet: [type %s] [content %s] [%d byte(s)]\n",
			frame_type(buffer), packet_content(buffer, length), res);
}

int llopen(char *serial_port, char timeout, int mode)
{
	int fd = -1;
	struct termios newtio;

	fd = open(serial_port, O_RDWR | O_NOCTTY);

	if (fd < 0) {
		perror(serial_port);
		return -1;
	}

	if (tcgetattr(fd, &oldtio) == -1) {
		perror("tcgetattr");
		return -1;
	}

	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0; //OPOST;
	//newtio.c_lflag &= ~(ICANON | ECHO);
	newtio.c_lflag = 0;
	newtio.c_cc[VTIME] = 3;
	newtio.c_cc[VMIN] = 0;

	if (tcflush(fd, TCIOFLUSH) != 0) {
		perror("tcflush");
		return -1;
	}

	if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
		perror("tcsetattr");
		return -1;
	}

	return fd;
}

int llclose(int fd)
{
	sleep(1);
	if (tcsetattr(fd, TCSANOW, &oldtio) == -1) {
		perror("tcsetattr");
		return -1;
	}
	close(fd);

	return 0;
}
