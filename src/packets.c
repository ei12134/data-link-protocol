#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>

#include "data_link.h"
#include "byte.h"
#include "packets.h"

int max_data_transfer = FRAME_SIZE;

size_t real_file_bytes = 0;
size_t received_file_bytes = 0;
size_t lost_packets = 0;
size_t duplicated_packets = 0;

struct connection g_connections[MAX_FD];

int send_file(char *port, struct file *file, int max_send_attempts)
{
	fprintf(stderr, "send_file\n");
	int fd = 0;
	struct connection* connection;
	int attempts_left = max_send_attempts;
	int state = SND_OPEN_CONNECTION;
	size_t num_data_bytes_sent = 0;
	size_t sequence_number = 0;

	while (attempts_left) {
		switch (state) {
		case SND_OPEN_CONNECTION:
			fprintf(stderr, "open connection\n");
			if ((fd = llopen(port, 1)) > 0) {
				connection = &g_connections[fd];
				attempts_left = max_send_attempts;
				state = SND_START_CONTROL_PACKET;
			} else {
#ifdef APPLICATION_LAYER_DEBUG_MODE
				fprintf(stderr, "llopen() returned an error code\n");
				fprintf(stderr, "\t%d attempts left\n", attempts_left - 1);
#endif
				state = SND_OPEN_CONNECTION;
				retry(&attempts_left);
			}
			break;
		case SND_START_CONTROL_PACKET:
			fprintf(stderr, "start control packet\n");
			if (send_control_packet(connection, file, control_field_start)
					< 0) {
#ifdef APPLICATION_LAYER_DEBUG_MODE
				fprintf(stderr,
						"start of transmission send_start_control_packet() returned an error code\n");
				fprintf(stderr, "\t%d attempts left\n", attempts_left - 1);
#endif
				retry(&attempts_left);
			} else {
				attempts_left = max_send_attempts;
				state = SND_DATA_PACKETS;
			}
			break;
		case SND_DATA_PACKETS:
			fprintf(stderr, "send data packets\n");
			if (send_data_packets(connection, file, &num_data_bytes_sent,
					&sequence_number) < 0) {
#ifdef APPLICATION_LAYER_DEBUG_MODE
				fprintf(stderr, "send_data_packets() returned an error code\n");
				fprintf(stderr, "\t%d attempts left\n", attempts_left - 1);
#endif
				retry(&attempts_left);
			} else {
				attempts_left = max_send_attempts;
				state = SND_CLOSE_CONTROL_PACKET;
			}
			break;
		case SND_CLOSE_CONTROL_PACKET:
			if (send_control_packet(connection, file, control_field_end) < 0) {
#ifdef APPLICATION_LAYER_DEBUG_MODE
				fprintf(stderr,
						"end of transmission send_control_packet() returned an error code");
				fprintf(stderr, "\t%d attempts left\n", attempts_left - 1);
#endif
				retry(&attempts_left);
			} else {
				attempts_left = max_send_attempts;
				state = SND_CLOSE_CONNECTION;
			}
			break;
		case SND_CLOSE_CONNECTION:
			if (llclose(fd) == 0) {
				return 0;
			} else {
				state = RCV_CLOSE_CONNECTION;
				retry(&attempts_left);
			}
			break;
		default:
			return -1;
			break;
		}
	}
	return -1;
}

int send_control_packet(struct connection* connection, struct file *file,
		byte control_field)
{
	// 5 bytes plus 2 specific data type sizes (value fields)
	size_t control_packet_size = (5 + sizeof(size_t)
			+ ((strlen(file->name) + 1) * sizeof(char)));
	if (control_packet_size > connection->packet_size) {
		fprintf(stderr, "control_packet_size (%zu) > (%zu) allowed packet size",
				control_packet_size, connection->packet_size);
		return -1;
	}

#ifdef APPLICATION_LAYER_DEBUG_MODE
	fprintf(stderr, "send_control_packet()\n");
	fprintf(stderr, "\tcontrol_field=%d\n", control_field);
	fprintf(stderr, "\tcontrol_packet_size=%zu\n", control_packet_size);
	fprintf(stderr, "\tpacket_size=%zu\n", (connection->packet_size));
	fprintf(stderr, "\tl1=%zu\n", sizeof(size_t));
	fprintf(stderr, "\tfile_size=%zu\n", file->size);
	fprintf(stderr, "\tname=%s\n", file->name);
	fprintf(stderr, "\tl2=%zu\n", strlen(file->name));
#endif

	byte* control_packet;
	if ((control_packet = malloc(control_packet_size * sizeof(byte))) == NULL ) {
		perror("send_control_packet() control_packet malloc error");
		return -1;
	}
	control_packet[control_field_index] = control_field;

	// TLV (file size)
	size_t v1_length = sizeof(size_t);
	control_packet[control_packet_t1_index] = control_packet_tlv_type_filesize;
	control_packet[control_packet_l1_index] = v1_length;

#ifdef APPLICATION_LAYER_DEBUG_MODE
	fprintf(stderr, "\tv1_length=%zu\n", v1_length);
#endif

	memcpy(control_packet + control_packet_v1_index, &(file->size), v1_length);

	// TLV (file name)
	size_t v2_length = strlen(file->name);
	size_t control_packet_t2_index = 4 + v1_length;
	size_t control_packet_l2_index = control_packet_t2_index + 1;
	size_t control_packet_v2_index = control_packet_l2_index + 1;

	control_packet[control_packet_t2_index] = control_packet_tlv_type_name;
	control_packet[control_packet_l2_index] = v2_length;
	memcpy(control_packet + control_packet_v2_index, (byte*) file->name,
			v2_length);
//	control_packet[control_packet_l2_index + v2_length] = '\0';

	if (transmitter_write(connection, control_packet, control_packet_size)
			< 0) {
		free(control_packet);
		return -1;
	}

	free(control_packet);
	return 0;
}

int send_data_packets(struct connection* connection, struct file* file,
		size_t* num_data_bytes_sent, size_t* sequence_number)
{
	fprintf(stderr, "send_data_packets\n");
	byte* file_data_pointer = (byte*) file->data;
	const byte* eof_data_pointer = ((byte*) file->data
			+ file->size * sizeof(char));

	for (size_t i = 0; i < *num_data_bytes_sent; i++)
		file_data_pointer++;

	while (file_data_pointer < eof_data_pointer) {
		fprintf(stderr, "while (file_data_pointer < eof_data_pointer)\n");
		size_t max_data_size = connection->packet_size
				- data_packet_header_size;
		if (max_data_transfer > 0 && max_data_transfer < max_data_size) {
			max_data_size = max_data_transfer;
		}

		size_t remaining_data_bytes = file->size - *num_data_bytes_sent;
		size_t remainder = remaining_data_bytes % (max_data_size);
		size_t data_bytes_to_send =
				remainder == 0 ? (max_data_size) : remainder;

		size_t data_packet_size = data_bytes_to_send + data_packet_header_size;

		byte* data_packet;
		if ((data_packet = malloc(data_packet_size * sizeof(byte))) == NULL ) {
			perror("send_control_packet() data_packet malloc error");
			return -1;
		}

		data_packet[control_field_index] = control_field_data;
		data_packet[data_packet_sequence_number_index] = (*sequence_number)
				% sequence_number_modulus;
		(*sequence_number)++;
		data_packet[data_packet_l2_index] = (data_bytes_to_send / 256);
		data_packet[data_packet_l1_index] = (data_bytes_to_send % 256);
		//fprintf(stderr,"sending packet %zu\n",data_packet_sequence_number_index);
		fprintf(stderr, "sequence_number: %ld\n", *sequence_number);

		for (size_t i = 0;
				file_data_pointer < eof_data_pointer && i < data_bytes_to_send;
				i++) {
			data_packet[i + data_packet_header_size] =
					(byte) file->data[*num_data_bytes_sent];

			file_data_pointer++;
			(*num_data_bytes_sent)++;
		}

		if (transmitter_write(connection, data_packet, data_packet_size) < 0) {
			free(data_packet);
			fprintf(stderr, "transmitter write returned negative\n");
			return -1;
		}

		free(data_packet);
	}
	return 0;
}

int receive_file(char *port, int max_receive_attempts)
{
	int fd = 0;
	int attempts_left = max_receive_attempts;
	char *file_name;
	size_t file_size;
	int state = RCV_OPEN_CONNECTION;

	while (attempts_left) {
		switch (state) {
		case RCV_OPEN_CONNECTION:
			fprintf(stderr, "opening connection..\n");
			if ((fd = llopen(port, 0)) > 0) {
				state = RCV_START_CONTROL_PACKET;
				attempts_left = max_receive_attempts;
			} else {
				retry(&attempts_left);
			}
			break;

		case RCV_START_CONTROL_PACKET:
			fprintf(stderr, "expecting control packet\n");
			if (receive_start_control_packet(fd, &file_name, &file_size) < 0) {
#ifdef APPLICATION_LAYER_DEBUG_MODE
				fprintf(stderr,
						"receive_start_control_packet() returned an error code\n");
				fprintf(stderr, "\t%d attempts left\n", attempts_left - 1);
#endif
				retry(&attempts_left);
				break;
			} else {
				state = RCV_DATA_PACKETS;
				attempts_left = max_receive_attempts;
				real_file_bytes = file_size;
			}
			break;

		case RCV_DATA_PACKETS:
			fprintf(stderr, "expecting data packet\n");
			if (receive_data_packets(fd, file_name, file_size, attempts_left)
					< 0) {
				receiver_stats();
				return -1;
				break;
			} else {
				state = RCV_CLOSE_CONNECTION;
				attempts_left = max_receive_attempts;
			}
			break;

		case RCV_CLOSE_CONNECTION:
			if (llclose(fd) == 0) {
				receiver_stats();
				return 0;
			} else {
				state = RCV_CLOSE_CONNECTION;
				retry(&attempts_left);
			}
			break;
		default:
			receiver_stats();
			return -1;
		}
	}
	return -1;
}

int receive_start_control_packet(const int fd, char **file_name,
		size_t *file_size)
{
	byte *control_packet;
	int control_packet_length = 0;

	if ((control_packet_length = llread(fd, &control_packet)) < 0) {
		free(control_packet);
		return -1;
	}

	byte control_field = control_packet[control_field_index];
	if (control_field == control_field_start) {
		return parse_control_packet(control_packet_length, control_packet,
				file_name, file_size);
	}

#ifdef APPLICATION_LAYER_DEBUG_MODE
	fprintf(stderr, "receive_data_packet(): bad control field value\n");
#endif

	free(control_packet);
	return -1;
}

int receive_data_packets(const int fd, char* file_name, size_t file_size,
		int attempts_left)
{
	FILE* received_file = fopen(file_name, "w");
	int receive_return_value = 1;
	size_t sequence_number = 0;

#ifdef APPLICATION_LAYER_DEBUG_MODE
	fprintf(stderr, "receive_data_packets()\n");
	fprintf(stderr, "\tfile_name=%s\n", file_name);
	fprintf(stderr, "\tfile_size=%zu\n", file_size);
#endif

	while (receive_return_value > 0 && attempts_left > 0) {

		char *file_data;

		if ((receive_return_value = receive_data_packet(fd, &file_data,
				received_file_bytes, &sequence_number)) < 0) {
#ifdef APPLICATION_LAYER_DEBUG_MODE
			fprintf(stderr, "receive_data_packet() returned an error code\n");
			fprintf(stderr, "\t%d attempts left\n", attempts_left - 1);
#endif
			retry(&attempts_left);
			receive_return_value = 1;
		} else {
			sequence_number++;
			received_file_bytes += receive_return_value;

#ifdef APPLICATION_LAYER_DEBUG_MODE
			fprintf(stderr, "\treceive_return_value=%d\n", receive_return_value);
			fprintf(stderr, "\treceived_file_bytes=%zu\n", received_file_bytes);
#endif

			if ((fwrite(file_data, sizeof(char), receive_return_value,
					received_file)) < 0) {
				fprintf(stderr, "Error: file write error\n");
				return -1;
			}

			if (receive_return_value > 0) {
				free(file_data);
			}
		}
	}

#ifdef APPLICATION_LAYER_DEBUG_MODE
	fprintf(stderr,"receive_data_packets()\n");
	fprintf(stderr,"\tfile_data_length=%zu\n", received_file_bytes);
#endif

	if (attempts_left <= 0) {
		return -1;
	}

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

	size_t *file_size_tmp;

	if ((file_size_tmp = malloc(sizeof(size_t))) == NULL ) {
		perror("parse_control_packet() file_size_tmp malloc error");
		return -1;
	}

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
		free(file_size_tmp);
		return -1;
	}

	size_t v2_length = *(control_packet + control_packet_l2_index);

	if ((*file_name = malloc(v2_length * sizeof(char))) == NULL ) {
		perror("parse_control_packet() file_name malloc error");
		free(file_size_tmp);
		return -1;
	}

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
		char **data, size_t* sequence_number)
{
	int data_size = data_packet[data_packet_l2_index] * 256
			+ data_packet[data_packet_l1_index];
#ifdef APPLICATION_LAYER_DEBUG_MODE
	fprintf(stderr, "parse_data_packet()\n");
	fprintf(stderr, "\tcontrol_field=%d\n", data_packet[control_field_index]);
	fprintf(stderr, "\tsequence_number=%d\n",
			data_packet[data_packet_sequence_number_index]);
	fprintf(stderr, "\tdata_size=%d\n", data_size);
#endif

	if ((*data = malloc(sizeof(char) * data_size)) == NULL ) {
		perror("parse_data_packet() data malloc error");
		return -1;
	}

	memcpy(*data, (data_packet + data_packet_header_size * sizeof(byte)),
			data_size);

	size_t received_sequence_number =
			data_packet[data_packet_sequence_number_index];
	size_t expected_sequence_number = *sequence_number
			% sequence_number_modulus;
	if (received_sequence_number != expected_sequence_number) {
#ifdef APPLICATION_LAYER_DEBUG_MODE
		fprintf(stderr, "bad packet sequence number: (received %zu) <-> (expected %zu)\n", received_sequence_number, expected_sequence_number);
		free(data_packet);
		return -1;
#endif
		if (received_sequence_number > expected_sequence_number) {

			while (*sequence_number % sequence_number_modulus
					!= received_sequence_number) {
				(*sequence_number)++;
				lost_packets++;
			}
		} else {
			duplicated_packets++;
			*sequence_number = expected_sequence_number;
		}
		//		free(data_packet);
//		free(*data);
//		return -1;
	}
	free(data_packet);
	return data_size;
}

int llread(const int fd, byte **packet)
{
	struct connection* c = &g_connections[fd];

// maximum size of a packet
	size_t packet_size = c->packet_size * sizeof(byte);

	if ((*packet = malloc(packet_size)) == NULL ) {
		perror("llread() packet malloc error");
		return -1;
	}

	int packet_length = 0;
	if (c->is_active) {
		if ((packet_length = receiver_read(c, *packet, packet_size,
				NUM_FRAMES_PER_CALL)) < 0) {
			fprintf(stderr, "llread(): error in receiver_read()\n");
			free(*packet);
			return -1;
		}
	} else {
		fprintf(stderr, "llread(): connection is not active\n");
		free(*packet);
		return -1;
	}

	return packet_length;
}

int receive_data_packet(const int fd, char **file_data,
		size_t received_file_bytes, size_t* sequence_number)
{
	byte *data_packet;
	int data_packet_length = 0;

	if ((data_packet_length = llread(fd, &data_packet)) < 0) {
		return -1;
	}

#ifdef APPLICATION_LAYER_DEBUG_MODE
	fprintf(stderr, "receive_data_packet()\n");
	fprintf(stderr, "\treceived_data_bytes=%d\n", data_packet_length);
#endif

	byte control_field = data_packet[control_field_index];
	if (control_field == control_field_data) {
#ifdef APPLICATION_LAYER_DEBUG_MODE
		fprintf(stderr, "\tdata packet\n");
#endif

		return parse_data_packet(data_packet_length, data_packet, file_data,
				sequence_number);
	} else if (control_field == control_field_end) {
#ifdef APPLICATION_LAYER_DEBUG_MODE
		fprintf(stderr, "\tend control packet\n");
#endif
		char* file_name;
		size_t file_size;
		if (parse_control_packet(data_packet_length, data_packet, &file_name,
				&file_size) == 0) {
			return 0;
		} else {
			return -1;
		}
	}

	fprintf(stderr, "receive_data_packet(): bad control field value\n");
	free(data_packet);
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

void receiver_stats()
{
	fprintf(stdout, "Receiver statistics\n");
	fprintf(stdout, "\treceived file bytes/file bytes:%zu/%zu\n",
			received_file_bytes, real_file_bytes);
	fprintf(stdout, "\tlost packets:%zu\n", lost_packets);
	fprintf(stdout, "\tduplicated packets:%zu\n", duplicated_packets);
}

void retry(int* attempt)
{
	(*attempt)--;
	if (*attempt <= 0)
		return;
#ifdef APPLICATION_LAYER_DEBUG_MODE
	fprintf(stderr, "\tnew attempt in 5 seconds .");
#endif
	sleep(1);
#ifdef APPLICATION_LAYER_DEBUG_MODE
	fprintf(stderr, " .");
#endif
	sleep(1);
#ifdef APPLICATION_LAYER_DEBUG_MODE
	fprintf(stderr, " .");
#endif
	sleep(1);
#ifdef APPLICATION_LAYER_DEBUG_MODE
	fprintf(stderr, " .");
#endif
	sleep(1);
#ifdef APPLICATION_LAYER_DEBUG_MODE
	fprintf(stderr, " .\n");
#endif
	sleep(1);
}
