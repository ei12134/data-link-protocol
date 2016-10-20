#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "data_link.h"
#include "byte.h"
#include "packets.h"

#define MAX_FD 10
#define FRAME_SIZE LL_MAX_PAYLOAD_UNSTUFFED
#define NUM_FRAMES_PER_CALL 1
#define TIMEOUT_S 5
#define MICRO_TIMEOUT_DS 5
#define NUM_RETRANSMISSIONS 3
#define CLOSE_WAIT_TIME 5
#define BUFSIZE (8*1024*1024)

struct connection g_connections[MAX_FD];

int send_file(char *port, struct file *file)
{
	// create connection
	int fd = llopen(port, 1);
	struct connection* conn = &g_connections[fd];

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
	int seq = 0;
	size_t num_data_bytes_sent = 0;
	byte* file_data_pointer = (byte*) file->data;
	const byte* eof_data_pointer = ((byte*) file->data
			+ file->size * sizeof(char));

	while (file_data_pointer < eof_data_pointer) {
		size_t max_data_size = conn->packet_size - packet_data_header_size;
		size_t remaining_data_bytes = file->size - num_data_bytes_sent;
		size_t remainder = remaining_data_bytes % (max_data_size);
		size_t data_bytes_to_send =
				remainder == 0 ? (max_data_size) : remainder;

		size_t data_packet_size = data_bytes_to_send + packet_data_header_size;
		byte* data_packet = malloc((data_packet_size) * sizeof(byte));

		data_packet[0] = control_field_data;
		data_packet[1] = seq++ % 255;
		data_packet[2] = data_bytes_to_send / 256;
		data_packet[3] = data_bytes_to_send % 256;

		for (size_t i = 0;
				file_data_pointer < eof_data_pointer && i < data_bytes_to_send;
				i++, file_data_pointer++, num_data_bytes_sent++) {
			data_packet[i + packet_data_header_size] =
					(byte) file->data[num_data_bytes_sent];
		}

		if (transmitter_write(conn, data_packet,
				(data_bytes_to_send + packet_data_header_size)) < 0) {
			return -1;
		}

		free(data_packet);
	}
// send close control packet

// end session
	fprintf(stderr, "attempt to disconnect\n");
	return llclose(fd);
}

int receive_file(char *port)
{
	int fd = llopen(port, 0);

	FILE* output_file = fopen("imagem.gif", "w");
	int ret = 0;
	int file_data_length = 1;

	while (file_data_length > 0) {

		char *file_data;

		if ((file_data_length = llread(fd, &file_data)) < 0) {
			fprintf(stderr, "netlink: closing prematurely\n");
			ret = -1;
			llclose(fd);
			exit(EXIT_FAILURE);
		}
		if (1) {
			printf("%.*s\n", file_data_length, file_data);
		}
		if ((ret = fwrite(file_data, sizeof(char), file_data_length,
				output_file)) < 0) {
			fprintf(stderr, "netlink: file write error\n");
			return -1;
		}

		if (file_data_length > 0) {
			free(file_data);
		}
	}

	printf("debug: num_bytes=%d\n", file_data_length);

	if (fclose(output_file) < 1) {
		ret = -1;
	}

	return llclose(fd);
}

int parse_data_packet(const int data_packet_length, byte *data_packet,
		char **data)
{
	int data_size = data_packet[l2_index] * 256 + data_packet[l1_index];
	fprintf(stderr, "  parse_data_packet()\n");
	fprintf(stderr, "\tcontrol_field=%d\n", data_packet[control_field_index]);
	fprintf(stderr, "\tsequence_number=%d\n",
			data_packet[sequence_number_index]);
	fprintf(stderr, "\tdata_size=%d\n", data_size);
	*data = malloc(
			sizeof(char) * (data_packet_length - packet_data_header_size));
	memcpy(*data, (data_packet + packet_data_header_size * sizeof(byte)),
			data_size);
	free(data_packet);
	return data_size;
}

int llread(const int fd, char **data)
{
	struct connection* conn = &g_connections[fd];

	// maximum size of a packet
	byte *data_packet = malloc(sizeof(byte) * conn->packet_size);

	int data_packet_length = 0;
	if (conn->is_active) {
		if ((data_packet_length = receiver_read(conn, data_packet,
				conn->packet_size, NUM_FRAMES_PER_CALL)) < 0) {
			fprintf(stderr, "llread(): there was an error\n");
			return -1;
		}
	} else
		return -1;

	byte control_field = data_packet[control_field_index];

	if (control_field == control_field_data)
		return parse_data_packet(data_packet_length, data_packet, data);
	else {
		fprintf(stderr, "  llread() bad packet control_field\n");
		return 0;
	}
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

int llclose(const int fd)
{
	return disconnect(&g_connections[fd]);
}

