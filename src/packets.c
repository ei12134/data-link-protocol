#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "data_link.h"
#include "byte.h"
#include "packets.h"

#define MAX_FD 10
#define FRAME_SIZE LL_MAX_PAYLOAD_UNSTUFFED
#define NUM_FRAMES_PER_CALL 10
#define TIMEOUT_S 5
#define MICRO_TIMEOUT_DS 5
#define NUM_RETRANSMISSIONS 3
#define CLOSE_WAIT_TIME 2

int TRANSMITTER;

struct connection g_connections[MAX_FD];

int send_file(char *port, struct file *file)
{
	// create connection
//	struct connection conn;
//	strcpy(conn.port, port);
//	conn.frame_size = FRAME_SIZE;
//	conn.micro_timeout_ds = MICRO_TIMEOUT_DS;
//	conn.timeout_s = TIMEOUT_S;
//	conn.num_retransmissions = NUM_RETRANSMISSIONS;
//	conn.close_wait_time = CLOSE_WAIT_TIME;

	int fd = llopen(port, 1);
	struct connection* conn = &g_connections[fd];
	printf("0\n");

//	if (transmitter_connect(&conn) < 0) {
//		return -1;
//	}
//
//	// send start control packet
//	struct control_packet scp = { .control_field = control_field_start, .t1 =
//			TLV_TYPE_FILESIZE};
//	scp.v1 = &file->size;
//	scp.l1 = sizeof(size_t);
//	scp.t2 = TLV_TYPE_NAME;
//	scp.l2 = strlen(file->name);
//	scp.v2 = file->name;
//
//	size_t s = sizeof(struct control_packet) -2*sizeof(byte*) + sizeof(char)*(scp.l2+1) + sizeof(size_t);
//	byte a[s];
//	memcpy(a,scp.control_field,sizeof(byte));
//	memcpy(a+sizeof(byte),scp.t1,sizeof(byte));
//	memcpy(a,scp.l1,sizeof(byte))
//	memcpy(a,scp.v1,sizeof(byte*))
//	memcpy(a,scp.t2,sizeof(byte));
//	memcpy(a,scp.l2,sizeof(byte));
//	memcpy(a,scp.v2,sizeof(byte*));
////	memcpy(scp + sizeof scp.,                    s.member2, sizeof s.member2);
////	memcpy(scp + sizeof s.member1 + sizeof s.member2, s.member3, sizeof s.member3);

	// send data packets
	byte* buff = malloc(conn->packet_size * sizeof(byte));
	byte* p = (byte*) file->data;
	int seq = 0;

	while (p < ((byte*) file->data + file->size * sizeof(char))) {
//		// send data packet
//		struct data_packet dpk;
//		dpk.control_field = control_field_data;
//
//		dpk.sequence_number = seq++ % 255;
//
//		size_t remainder = file->size % conn->packet_size;
//		size_t num_bytes = remainder == 0 ? conn->packet_size : remainder;
//
//		dpk.l2 = num_bytes / 256;
//		dpk.l1 = num_bytes % 256;

		size_t remainder = file->size % (conn->packet_size - 4);
		size_t num_bytes = remainder == 0 ? (conn->packet_size - 4) : remainder;
		buff[0] = control_field_data;
		buff[1] = seq++ % 255;
		buff[2] = num_bytes / 256;
		buff[3] = num_bytes % 256;

		for (int i = 0; i < file->size; ++i) {
			buff[i + 4] = file->data[i];
		}

		if (transmitter_write(conn, buff, num_bytes + PACKET_DATA_HEADER_SIZE) < 0) {
			return -1;
		}

		p += num_bytes;
	}

// send close control packet

// end session
	return llclose(fd);
}

int llread(const int fd, const char *msg, int buff_remaining)
{ //	return p - (byte*) buffer;
//}
	struct connection* conn = &g_connections[fd];
	unsigned long counter = 0;

	byte* p = (byte*) msg;
	while (conn->is_active && buff_remaining > 0) {
		time_t t = time(NULL );

		int num_bytes;
		if ((num_bytes = receiver_read(conn, p, buff_remaining,
				NUM_FRAMES_PER_CALL)) < 0) {
			fprintf(stderr, "llread(): there was an error\n");
			break;
		}
		if (num_bytes > buff_remaining) {
			fprintf(stderr, "too many bytes written... %d\n", __LINE__);
		}
		counter++;
		p += num_bytes;
		buff_remaining -= num_bytes;

		print_status(t, num_bytes, counter);
	}
	return p - (byte*) msg;
}

int llopen(char *port, int transmitter)
{
	struct connection conn;
	strcpy(conn.port, port);
	conn.frame_size = FRAME_SIZE;
	conn.micro_timeout_ds = MICRO_TIMEOUT_DS;
	conn.timeout_s = TIMEOUT_S;
	conn.num_retransmissions = NUM_RETRANSMISSIONS;
	conn.close_wait_time = CLOSE_WAIT_TIME;
	conn.packet_size = FRAME_SIZE;

	if ((conn.is_transmitter = transmitter)) {
		if (transmitter_connect(&conn) < 0) {
			return -1;
		}
	} else {
		if (receiver_listen(&conn)) {
			return -1;
		}
	}

	g_connections[conn.fd] = conn;
	return conn.fd;
}
//	return p - (byte*) buffer;
//}
//int connect(char *port, int transmitter)
//{
//	struct connection conn;
//	strcpy(conn.port, port);
//	conn.packet_size = PACKET_SIZE;
//	conn.micro_timeout_ds = MICRO_TIMEOUT_DS;
//	conn.timeout_s = TIMEOUT_S;
//	conn.num_retransmissions = NUM_RETRANSMISSIONS;
//	conn.close_wait_time = CLOSE_WAIT_TIME;
//
//	if ((conn.is_transmitter = transmitter)) {
//		if (transmitter_connect(&conn) < 0) {
//			return -1;
//		}
//	} else {
//		if (receiver_listen(&conn)) {
//			return -1;
//		}
//	}
//
//	g_connections[conn.fd] = conn;
//	return conn.fd;
//}

void print_status(time_t t0, size_t num_bytes, unsigned long counter)
{
	double dt = difftime(time(NULL ), t0);
	double speed = ((double) (num_bytes * 8)) / dt;
	fprintf(stderr, "----------------------\n");
	fprintf(stderr,
			"Link layer transmission %ld: %lf bit per sec; %ldB of data\n",
			counter, speed, num_bytes);
	fprintf(stderr, "----------------------\n");
}

int llwrite(const int fd, const char *buffer, int length)
{
	struct connection *conn = &g_connections[fd];
	unsigned long counter = 0;

	byte* p = malloc(conn->packet_size * sizeof(byte));
	while (length > 0) {
		time_t t = time(NULL );

		size_t remainder = length % conn->frame_size;
		size_t num_bytes = remainder == 0 ? conn->frame_size : remainder;
		//unsigned long num_bytes = ((length-1) % conn->packet_size) + 1;

		if (transmitter_write(conn, p, num_bytes) < 0) {
			return -1;
		}
		counter++;
//		memcpy() ?????
		p += num_bytes;
		length -= num_bytes;

		print_status(t, num_bytes, counter);
	}
	free(p);

	return 0;
}

//int llread(const int fd, const char *buffer, int buff_remaining)
//{
//	struct connection* conn = &g_connections[fd];
//	unsigned long counter = 0;
//
//	byte* p = (byte*) buffer;
//	while (conn->is_active && buff_remaining > 0) {
//		time_t t = time(NULL );
//
//		int num_bytes;
//		if ((num_bytes = receiver_read(conn, p, buff_remaining,
//				NUM_FRAMES_PER_CALL)) < 0) {
//			fprintf(stderr, "llread(): there was an error\n");
//			break;
//		}
//		if (num_bytes > buff_remaining) {
//			fprintf(stderr, "too many bytes written... %d\n", __LINE__);
//		}
//		counter++;
//		memcpy(buffer, p, num_bytes);
//		p += num_bytes;
//		buff_remaining -= num_bytes;
//		print_status(t, num_bytes, counter);
//	}
//	return p - (byte*) buffer;
//}

int llclose(int fd)
{
	return disconnect(&g_connections[fd]);
}

