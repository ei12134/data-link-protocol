#ifndef PACKETS_H_
#define PACKETS_H_

#include "byte.h"
#include "file.h"

const static int control_field_data = 1;
const static int control_field_start = 2;
const static int control_field_end = 3;

struct data_packet {
	byte control_field; // 1 - data
	byte sequence_number;
	byte l2;
	byte l1;
	byte *data;
};

struct control_packet {
	byte control_field; // 2 - start ; 3 - end
	byte t1; // 0 - file size
	byte l1; // v1 length
	byte *v1; // file size
	byte t2; // 0 - filename
	byte l2; // v2 length
	byte *v2; // filename
};

//create_data_packet();
//create_control_packet();

int send_file(int fd, struct file *file);
//int receive_file(int fd)

int llopen(char *port, int transmitter);
int llwrite(const int fd, const char *buffer, int length);
int llread(const int fd, const char *buffer, int buff_remaining);
int llclose(int fd);

#endif // PACKETS_H_
