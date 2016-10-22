#include <stdio.h>

#include "serial_port.h"
#include "data_link.h"
#include "data_link_codes.h"
#include "data_link_io.h"
#include <string.h>
#include <stdlib.h>

#define TRUE 1
#define FALSE 1

static long g_use_limited_rejected_retries = 0; // true or false

byte data_reply_byte(unsigned long frame_number, int accepted)
{
	return (accepted ? C_RR : C_REJ) | ((accepted) ? 0 : (1 << 5));
}

byte data_control_byte(unsigned long frame_number)
{
	return (frame_number % 2 == 0) ? 0 : (1 << 5);
}

static int handle_disconnect(struct connection* conn)
{
	int ret = 0;

	int ntries = conn->num_retransmissions;
	while (1) {
		struct frame reply;
		if ((ntries = f_send_acknowledged_frame(conn->fd, ntries,
				conn->timeout_s, DISC, &reply)) < 0) {
			ret = -1;
			break;
		}
		if (reply.control == C_UA) {
			break;
		}
	}

	conn->is_active = 0;
	if (serial_port_close(conn->fd, 0) < 0 || ret < 0) {
		return -1;
	}
	return 0;
}

/*
 * TRANSMITTER
 */

/** \brief Establish logical connection.
 *
 * Open serial port, send SET, receive UA. */
int transmitter_connect(struct connection* conn)
{
	conn->is_active = 0;
	conn->max_buffer_size = LL_MAX_PAYLOAD_STUFFED;
	conn->frame_number = 0;

	if ((conn->fd = serial_port_open(conn->port, conn->micro_timeout_ds)) < 0) {
#ifdef DATA_LINK_DEBUG_MODE
		fprintf(stderr,
				"transmitter_connect(): could not open %s\n",conn->port);
#endif
		return conn->fd;
	}

	/* Send SET frame and receive UA.  */
	int ntries = conn->num_retransmissions;
	while (1) {
		struct frame reply;
		if ((ntries = f_send_acknowledged_frame(conn->fd, ntries,
				conn->timeout_s, SET, &reply)) < 0) {
#ifdef DATA_LINK_DEBUG_MODE
			fprintf(stderr,"transmitter_connect(): no connection.\n");
#endif
			return -1;
		}

		if (reply.control == C_UA) {
			break;
		}
	}

	conn->is_active = 1;
#ifdef DATA_LINK_DEBUG_MODE
	fprintf(stderr,"Connection established.\n");
#endif
	return 0;
}

int transmitter_write(struct connection* conn, byte* data_packet, size_t size)
{
	struct frame out_frame = { .address = A, .control = data_control_byte(
			conn->frame_number), .size = size, .data = data_packet };

	printf(" ###################################### \n");
	printf("\n");
	printf(" BEGIN TRANSMIT %x\n", out_frame.control);
	printf("\n");
	printf(" ###################################### \n");

	byte success_rep = data_reply_byte(conn->frame_number, TRUE);
	byte rej_rep = data_reply_byte(conn->frame_number, FALSE);

	/* Send data frame and receive confirmation.  */
	int ntries = conn->num_retransmissions;
	while (1) {
		struct frame reply_frame;
		if ((ntries = f_send_acknowledged_frame(conn->fd, ntries,
				conn->timeout_s, out_frame, &reply_frame)) < 0) {
			return -1;
		}
		if (reply_frame.control == rej_rep) {
			if (g_use_limited_rejected_retries) {
				--ntries;
			}
		}
		if (reply_frame.control == success_rep) {
			break;
		}
	}

	conn->frame_number++;
	return 0;
}

/** \brief Establish logical connection.
 *
 * Open serial port, send SET, receive UA. */
int disconnect(struct connection* conn)
{
	int return_value = 0;

	/* Send DISC and receive DISC.  */
	int ntries = conn->num_retransmissions;
	while (1) {
		struct frame reply;
		if ((ntries = f_send_acknowledged_frame(conn->fd, ntries,
				conn->timeout_s, DISC, &reply)) < 0) {
			return_value = -1;
			break;
		}
		if (reply.control == C_DISC) {
			break;
		}
	}

	if (return_value >= 0) {
		if (f_send_frame(conn->fd, UA) != SUCCESS_CODE) {
			return_value = -1;
		}
	}

	conn->is_active = 0;

	// Close port
	if (serial_port_close(conn->fd, conn->close_wait_time) < 0) {
		return_value = -1;
	}

#ifdef DATA_LINK_DEBUG_MODE
	if (return_value < 0) {
		fprintf(stderr,"disconnect(): failed to close connection.\n");
	}
#endif
	return return_value;
}

/*
 * RECEIVER
 */

// TODO
int receiver_listen(struct connection* conn)
{
	conn->max_buffer_size = LL_MAX_PAYLOAD_STUFFED;
	conn->frame_number = 0;

	if ((conn->fd = serial_port_open(conn->port, conn->micro_timeout_ds)) < 0) {
#ifdef DATA_LINK_DEBUG_MODE
		fprintf(stderr,"listen(): could not open %s\n",conn->port);
#endif
		return conn->fd;
	}

	while (1) {
		struct frame in;
		if (f_receive_frame(conn->fd, &in, 0) == ERROR_CODE) {
			return -1;
		}
		if (in.control == C_SET) {
			f_send_frame(conn->fd, UA);
			conn->is_active = 1;
#ifdef DATA_LINK_DEBUG_MODE
			fprintf(stderr,"listen(): connection established.\n");
#endif
			return 0;
		}
	}
}

int receiver_read(struct connection* conn, byte *begin, size_t max_data_size,
		const int max_num_frames)
{
	int num_frames = 0;
	byte *p = begin;
	byte *end = begin + max_data_size;

	while (p < end && (num_frames < max_num_frames || max_num_frames == 0)) {
		struct frame in;
		in.data = p;
		in.max_data_size = end - p;
		Return_e ret = f_receive_frame(conn->fd, &in, 0);

		printf(" ###################################### \n");
		printf("\n");
		printf(" BEGIN RECEIVE %x\n", in.control);
		printf("\n");
		printf(" ###################################### \n");

		if (ret == ERROR_CODE) {
			return -1;
		} else if (ret == TIMEOUT_CODE) {
			break;
		} else if (ret == BADFRAME_CODE) {
#ifdef DATA_LINK_DEBUG_MODE
			fprintf(stderr,"receiver_read(): parsing: bad frame.\n");
#endif
			/*
			 * Send 'bad frame' acknowledgment.
			 */
			byte c_out = data_reply_byte(conn->frame_number, FALSE);
			if (f_send_frame(conn->fd, FRAME(c_out)) != SUCCESS_CODE) {
				break;
			}
		} else if (in.control == C_DISC) {
			handle_disconnect(conn);
			break;
		} else if (in.control != data_control_byte(conn->frame_number)) {
			/*
			 * Frame number mismatch.
			 */
#ifdef DATA_LINK_DEBUG_MODE
			fprintf(stderr,"receiver_read(): bad control byte.\n");
			fprintf(stderr,"receiver_read(): %x, %x.\n",
					in.control,data_control_byte(conn->frame_number));
#endif
			byte control = data_reply_byte(conn->frame_number, FALSE);
			// reject this frame
			if (f_send_frame(conn->fd, FRAME(control)) != SUCCESS_CODE) {
				break;
			}
		} else {
#ifdef DATA_LINK_DEBUG_MODE
			fprintf(stderr,"receiver_read(): received: \"%.*s\".\n",
					(int)in.size,p);
#endif

			num_frames++;
			p += in.size;

			byte control = data_reply_byte(conn->frame_number, TRUE);
			if (f_send_frame(conn->fd, FRAME(control)) != SUCCESS_CODE) {
				break;
			}
			conn->frame_number++;
		}
	}
	return p - begin;
}

// TODO
int wait_for_disconnect(struct connection* conn, int timeout)
{
	while (1) {
		struct frame in;
		f_receive_frame(conn->fd, &in, 0);
		if (in.control == C_DISC) {
			return handle_disconnect(conn);
		} else {
#ifdef DATA_LINK_DEBUG_MODE
			fprintf(stderr,
					"receiver_wait_disconnect(): frame ignored, C=%x.\n",
					in.control);
#endif
		}
	}
	return -1;
}

#if 0
// ------------------------

// Function to print the pack content
char* packet_content(const char* packet, const int size)
{
	const char *hex = "0123456789ABCDEF";
	char *content = (char *) malloc(sizeof(char) * (3 * size));
	char *pout = content;
	const char *pin = packet;

	int i;

	if (pout) {

		for (i = 0; i < size - 1; i++) {
			*pout++ = hex[(*pin>>4)&0xF];
			*pout++ = hex[(*pin++)&0xF];
			*pout++ = ':';
		}
		*pout++ = hex[(*pin>>4)&0xF];
		*pout++ = hex[(*pin++)&0xF];
		*pout = 0;
	}

	return content;
}

#endif
