#ifndef PACKETS_H_
#define PACKETS_H_

#include <time.h>
#include "byte.h"
#include "file.h"

const static byte control_field_data = 1;
const static byte control_field_start = 2;
const static byte control_field_end = 3;
const static byte tlv_type_filesize = 0;
const static byte tlv_type_name = 1;
const static size_t packet_data_header_size = 4;

const static size_t control_field_index = 0;
const static size_t sequence_number_index = 1;
const static size_t l2_index = 2;
const static size_t l1_index = 3;

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
int receive_file(char *port);

int parse_data_packet(const int data_packet_length, byte *data_packet,
		char **data);
void print_status(time_t t0, size_t num_bytes, unsigned long counter);

int connect(char *port, int transmitter);
int llopen(char *port, int transmitter);
int send_packet(const int fd, const char *buffer, int length);

int llread(const int fd, char **data);
int llclose(const int fd);

#endif // PACKETS_H_
