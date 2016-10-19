#ifndef PACKETS_H_
#define PACKETS_H_

#include <time.h>
#include "byte.h"
#include "file.h"

const static int control_field_data = 1;
const static int control_field_start = 2;
const static int control_field_end = 3;

const static int TLV_TYPE_FILESIZE = 0;
const static int TLV_TYPE_NAME = 1;
const static int PACKET_DATA_HEADER_SIZE = 4;

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
	byte t2; // 1 - filename
	byte l2; // v2 length
	byte *v2; // filename
};

//create_data_packet();
//create_control_packet();

int send_file(char *port, struct file *file);
//int receive_file(int fd)

void print_status(time_t t0, size_t num_bytes, unsigned long counter);

int connect(char *port, int transmitter);
int llopen(char *port, int transmitter);
int send_packet(const int fd, const char *buffer, int length);
int llread(const int fd, const char *buffer, int buff_remaining);
int llclose(int fd);

#endif // PACKETS_H_
