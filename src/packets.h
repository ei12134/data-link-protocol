#ifndef PACKETS_H_
#define PACKETS_H_

#include <time.h>
#include "byte.h"
#include "file.h"
#include "data_link.h"
#include "byte.h"
#include "packets.h"

extern int max_data_transfer;

#define MAX_FD 10

#define FRAME_SIZE LL_MAX_PAYLOAD_UNSTUFFED
#define NUM_FRAMES_PER_CALL 1
#define TIMEOUT_S 5
#define MICRO_TIMEOUT_DS 5
#define NUM_RETRANSMISSIONS 3
#define CLOSE_WAIT_TIME 3

// data packet general constants
const static byte control_field_data = 1;
const static byte control_field_start = 2;
const static byte control_field_end = 3;
const static size_t control_field_index = 0;
const static size_t sequence_number_modulus = 255;

// control data packet specific constants
const static byte control_packet_tlv_type_filesize = 0;
const static byte control_packet_tlv_type_name = 1;
const static size_t control_packet_t1_index = 1;
const static size_t control_packet_l1_index = 2;
const static size_t control_packet_v1_index = 3;

// data packet specific constants
const static size_t data_packet_header_size = 4;
const static size_t data_packet_sequence_number_index = 1;
const static size_t data_packet_l2_index = 2;
const static size_t data_packet_l1_index = 3;

// sender state machine
#define SND_OPEN_CONNECTION 0
#define SND_START_CONTROL_PACKET 1
#define SND_DATA_PACKETS 2
#define SND_CLOSE_CONTROL_PACKET 3
#define SND_CLOSE_CONNECTION 4

// receiver state machine
#define RCV_OPEN_CONNECTION 0
#define RCV_START_CONTROL_PACKET 1
#define RCV_DATA_PACKETS 2
#define RCV_CLOSE_CONNECTION 3

void print_status(time_t t0, size_t num_bytes, unsigned long counter);

int send_file(char *port, struct file *file, int send_attempts);
int send_control_packet(struct connection* connection, struct file *file,
		byte control_field);
int send_data_packets(struct connection* connection, struct file* file,
		size_t* num_data_bytes_sent, size_t* sequence_number);
int send_packet(const int fd, const char *buffer, int length);

int receive_file(char *port, int receive_attempts);
int receive_start_control_packet(const int fd, char **file_name,
		size_t *file_size);
int llread(const int fd, byte **packet);
int receive_data_packets(const int fd, char* file_name, size_t file_size,
		int attempts);
int parse_control_packet(const int control_packet_length, byte *control_packet,
		char **file_name, size_t *file_size);
int parse_data_packet(const int data_packet_length, byte *data_packet,
		char **data, size_t* sequence_number);

int receive_data_packet(const int fd, char **data, size_t received_file_bytes,
		size_t* sequence_number);
int llopen(char *port, int transmitter);
int connect(char *port, int transmitter);
void receiver_stats();
int llclose(const int fd);

void retry(int* attempt);

#endif // PACKETS_H_
