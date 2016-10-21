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
#define CLOSE_WAIT_TIME 3

struct connection g_connections[MAX_FD];

int send_file(char *port, struct file *file)
{
	// create connection
	int fd = llopen(port, 1);
	struct connection* connection = &g_connections[fd];

	if (connection->is_active) {
		// send start control packet
		if (send_start_control_packet(connection, file) < 0) {
			fprintf(stderr,
					"send_start_control_packet() returned an error code");
			return llclose(fd);
		}

		// send data packets
		if (send_data_packets(connection, file) < 0) {
			fprintf(stderr, "send_data_packets() returned an error code");
			return llclose(fd);
		}
	} else {
		fprintf(stderr, "send_file(): connection is not active\n");
	}

	// send close control packet

	// end session
	return llclose(fd);
}

int send_start_control_packet(struct connection* connection, struct file *file)
{
	// 5 bytes plus 2 specific data type sizes (value fields)
	size_t control_packet_size = (5 + sizeof(size_t)
			+ ((strlen(file->name) + 1) * sizeof(char)));
	if (control_packet_size > connection->packet_size) {
		fprintf(stderr, "control_packet_size > allowed packet size");
		return -1;
	}

#ifdef APPLICATION_LAYER_DEBUG_MODE
	fprintf(stderr, "  send_start_control_packet()\n");
	fprintf(stderr, "\tcontrol_packet_size=%zu\n", control_packet_size);
	fprintf(stderr, "\tpacket_size=%zu\n", (connection->packet_size));
	fprintf(stderr, "\tfile_size=%zu\n", file->size);
	fprintf(stderr, "\tname=%s\n", file->name);
#endif

	byte* control_packet = malloc(control_packet_size * sizeof(byte));

	control_packet[control_field_index] = control_field_start;

	// TLV (file size)
	size_t v1_length = sizeof(size_t);
	control_packet[control_packet_t1_index] = control_packet_tlv_type_filesize;
	control_packet[control_packet_l1_index] = v1_length;

#ifdef APPLICATION_LAYER_DEBUG_MODE
	fprintf(stderr, "\tv1_length=%zu\n", v1_length);
#endif

	memcpy(control_packet + control_packet_v1_index, &(file->size), v1_length);

	// TLV (file name)
	size_t v2_length = (strlen(file->name) + 1);
	size_t control_packet_t2_index = 4 + v1_length;
	size_t control_packet_l2_index = control_packet_t2_index + 1;
	size_t control_packet_v2_index = control_packet_l2_index + 1;

	control_packet[control_packet_t2_index] = control_packet_tlv_type_name;
	control_packet[control_packet_l2_index] = v2_length;
	memcpy(control_packet + control_packet_v2_index, (byte*) file->name,
			v2_length);

	if (transmitter_write(connection, control_packet, control_packet_size)
			< 0) {
		return -1;
	}
	free(control_packet);

	return 0;
}

int send_data_packets(struct connection* connection, struct file* file)
{
	// send data packets
	int sequence_number = 0;
	size_t num_data_bytes_sent = 0;
	byte* file_data_pointer = (byte*) file->data;
	const byte* eof_data_pointer = ((byte*) file->data
			+ file->size * sizeof(char));

	while (file_data_pointer < eof_data_pointer) {
		size_t max_data_size = connection->packet_size
				- data_packet_header_size;
		size_t remaining_data_bytes = file->size - num_data_bytes_sent;
		size_t remainder = remaining_data_bytes % (max_data_size);
		size_t data_bytes_to_send =
				remainder == 0 ? (max_data_size) : remainder;

		size_t data_packet_size = data_bytes_to_send + data_packet_header_size;
		byte* data_packet = malloc(data_packet_size * sizeof(byte));

		data_packet[control_field_index] = control_field_data;
		data_packet[data_packet_sequence_number_index] = (sequence_number++
				% 255);
		data_packet[data_packet_l2_index] = (data_bytes_to_send / 256);
		data_packet[data_packet_l1_index] = (data_bytes_to_send % 256);

		for (size_t i = 0;
				file_data_pointer < eof_data_pointer && i < data_bytes_to_send;
				i++) {
			data_packet[i + data_packet_header_size] =
					(byte) file->data[num_data_bytes_sent];

			file_data_pointer++;
			num_data_bytes_sent++;

		}

		if (transmitter_write(connection, data_packet, data_packet_size) < 0) {
			return -1;
		}

		free(data_packet);
	}
	return 0;
}

int receive_file(char *port)
{
	int fd = llopen(port, 0);

	char *file_name;
	size_t file_size;
	char* tmp_file_name = "pinguim.gif";

	if (receive_start_control_packet(fd, &file_name, &file_size) < 0) {
		fprintf(stderr,
				"receive_start_control_packet() returned an error code");
		return llclose(fd);
	}

	if (receive_data_packets(fd, tmp_file_name, file_size) < 0) {
		fprintf(stderr, "receive_data_packets() returned an error code");
		return llclose(fd);
	}

	return llclose(fd);
}

int receive_start_control_packet(const int fd, char **file_name,
		size_t *file_size)
{
	byte *control_packet;
	int control_packet_length = 0;

	if ((control_packet_length = receive_packet(fd, &control_packet)) < 0) {
		return -1;
	}

	byte control_field = control_packet[control_field_index];
	if (control_field == control_field_start) {
		return parse_control_packet(control_packet_length, control_packet,
				file_name, file_size);
	}
	fprintf(stderr, "receive_data_packet(): bad control field value\n");
	return -1;
}

int receive_data_packets(const int fd, char* file_name, size_t file_size)
{
	FILE* received_file = fopen(file_name, "w");
	int received_data_bytes = 0;
	int file_data_length = 0;

#ifdef APPLICATION_LAYER_DEBUG_MODE
	fprintf(stderr, "receive_data_packets()\n");
	fprintf(stderr, "\tfile_name=%s\n", file_name);
	fprintf(stderr, "\tfile_size=%zu\n", file_size);
#endif

	while (file_data_length < (int) file_size) {

		char *file_data;

		if ((received_data_bytes = receive_data_packet(fd, &file_data)) < 0) {
			fprintf(stderr, "netlink: closing prematurely\n");
			return llclose(fd);
		}
		file_data_length += received_data_bytes;

#ifdef APPLICATION_LAYER_DEBUG_MODE
		fprintf(stderr, "\treceived_data_bytes=%d\n", received_data_bytes);
		fprintf(stderr, "\tfile_data_length=%d\n", file_data_length);
#endif

		if ((fwrite(file_data, sizeof(char), received_data_bytes, received_file))
				< 0) {
			fprintf(stderr, "netlink: file write error\n");
			return -1;
		}

		if (file_data_length > 0) {
			free(file_data);
		}
	}

#ifdef APPLICATION_LAYER_DEBUG_MODE
	fprintf(stderr,"file_data_length=%d\n", file_data_length);
#endif

	return fclose(received_file);
}

int parse_control_packet(const int control_packet_length, byte *control_packet,
		char **file_name, size_t *file_size)
{
	// TLV (file size)
	if (control_packet[control_packet_t1_index]
			!= control_packet_tlv_type_filesize) {
		fprintf(stderr, "parse_control_packet(): bad type 1");
		return -1;
	}
	size_t v1_length = control_packet[control_packet_l1_index];

	if (v1_length != sizeof(size_t)) {
		fprintf(stderr, "parse_control_packet(): bad L1 - file size length");
		return -1;
	}

	size_t *file_size_tmp = malloc(sizeof(size_t));
	memcpy(file_size_tmp, (control_packet + control_packet_v1_index),
			v1_length);
	*file_size = *file_size_tmp;

	// TLV (file name)
	size_t control_packet_t2_index = control_packet_v1_index + v1_length + 1;
	size_t control_packet_l2_index = control_packet_t2_index + 1;
	size_t control_packet_v2_index = control_packet_l2_index + 1;

	byte t2 = *(control_packet + control_packet_t2_index);

	if (t2 != control_packet_tlv_type_name) {
		fprintf(stderr, "parse_control_packet(): bad type 2");
		return -1;
	}

	size_t v2_length = *(control_packet + control_packet_l2_index);
	*file_name = malloc(v2_length * sizeof(char));
	memcpy(*file_name, (control_packet + control_packet_v2_index), v2_length);

#ifdef APPLICATION_LAYER_DEBUG_MODE
	fprintf(stderr, "\tl1=%zu\n", v1_length);
	fprintf(stderr, "\tfile_size=%zu\n", *file_size);
	fprintf(stderr, "\tl2=%zu\n", v2_length);
	fprintf(stderr, "\tname=%s\n", *file_name);
#endif
	free(control_packet);
	return 0;
}

int parse_data_packet(const int data_packet_length, byte *data_packet,
		char **data)
{
	int data_size = data_packet[data_packet_l2_index] * 256
			+ data_packet[data_packet_l1_index];
	fprintf(stderr, "parse_data_packet()\n");
	fprintf(stderr, "\tcontrol_field=%d\n", data_packet[control_field_index]);
	fprintf(stderr, "\tsequence_number=%d\n",
			data_packet[data_packet_sequence_number_index]);
	fprintf(stderr, "\tdata_size=%d\n", data_size);
	*data = malloc(sizeof(char) * data_size);
	memcpy(*data, (data_packet + data_packet_header_size * sizeof(byte)),
			data_size);
	free(data_packet);
	return data_size;
}

int receive_packet(const int fd, byte **packet)
{
	struct connection* c = &g_connections[fd];

	// maximum size of a packet
	size_t packet_size = c->packet_size * sizeof(byte);
	*packet = malloc(packet_size);

	int packet_length = 0;
	if (c->is_active) {
		if ((packet_length = receiver_read(c, *packet, packet_size,
				NUM_FRAMES_PER_CALL)) < 0) {
			fprintf(stderr,
					"receive_packet(): error in the receiver_read() call\n");
			return -1;
		}
	} else {
		fprintf(stderr, "receive_packet(): connection is not active\n");
		return -1;
	}

	return packet_length;
}

int receive_data_packet(const int fd, char **data)
{
	byte *data_packet;
	int data_packet_length = 0;

	if ((data_packet_length = receive_packet(fd, &data_packet)) < 0) {
		return -1;
	}

#ifdef APPLICATION_LAYER_DEBUG_MODE
	fprintf(stderr, "receive_data_packet()");
	fprintf(stderr, "\treceived_data_bytes=%d\n", data_packet_length);
#endif

	byte control_field = data_packet[control_field_index];
	if (control_field == control_field_data) {
		return parse_data_packet(data_packet_length, data_packet, data);
	}

	fprintf(stderr, "receive_data_packet(): bad control field value\n");
	return -1;
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

int llclose(const int fd)
{
	return disconnect(&g_connections[fd]);
}

