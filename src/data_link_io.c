#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

#include "serial_port.h"
#include "data_link.h"
#include "data_link_io.h"
#include "data_link_codes.h"
#include "byte.h"

volatile int g_timeout_alarm = 0;
void set_timeout_alarm()
{
	g_timeout_alarm = 1;
}

static byte g_buffer[LL_MAX_FRAME_SZ]; /** Local array for frame building. */
static long g_sent_frame_counter = 0;
static long g_rec_frame_counter = 0;
static long g_header_bcc_error_counter = 0;
static long g_data_bcc_error_counter = 0;

struct frame FRAME(const byte control)
{
	struct frame super = { .address = A, .control = control, .size = 0 };
	return super;
}

void f_print_frame(const struct frame frame)
{
	fprintf(stderr, "Frame:\n");
	fprintf(stderr, "A:%o C:%o S:%zu\n", frame.address, frame.control,
			frame.size);
	if (frame.size > 0) {
		for (int i = 0; i < frame.size; i++) {
			putc(frame.data[i], stderr);
		}
		putc('\n', stderr);
	}
	putc('\n', stderr);
}

void f_dump_frame_buffer(const char *filename)
{
	FILE* f;
	if ((f = fopen(filename, "w")) == NULL ) {
		fprintf(stderr, "f_dump_frame_buffer(): file error: line: %d\n",
				__LINE__);
	} else if (fprintf(f, "%.*s", LL_MAX_FRAME_SZ, g_buffer) < 0) {
		fprintf(stderr, "f_dump_frame_buffer(): file error: line: %d\n",
				__LINE__);
	} else if (fclose(f) == EOF) {
		fprintf(stderr, "f_dump_frame_buffer(): file error: line: %d\n",
				__LINE__);
	}
}

/* Reads array and builds a "struct Frame* frame*" from it
 * - checks if its a supervision or data frame
 * - checks bcc 
 * - destuffs bytes
 * - returns SUCCESS_CODE, ERROR_CODE, or BADFRAME_CODE (when bcc is wrong, or
 *   data is too large)
 */
static Return_e parse_frame_from_array(struct frame* frame, byte *a)
{
#ifdef DATA_LINK_DEBUG_MODE
	fprintf(stderr,"  parse_frame_from_array(): entering function.\n");
	//f_dump_frame_buffer("FRAME");
#endif

	if (*a++ != FLAG) {
#ifdef DATA_LINK_DEBUG_MODE
		fprintf(stderr,"  parse_frame_from_array(): error: missing flag: \
                (line %d).\n",__LINE__);
#endif
		return ERROR_CODE;
	}
	for (int i = 0; i <= 2; i++) { // the next fields should not have a FLAG
		if (a[i] == FLAG) {
#ifdef DATA_LINK_DEBUG_MODE
			fprintf(stderr,"  parse_frame_from_array(): error: unexpected flag \
                    (line %d).\n",__LINE__);
#endif
			return BADFRAME_CODE;
		}
	}

	frame->address = *a++;
	// TODO: verificar que o 'control' é válido e sair antecipadamente
//	(accepted ? C_RR : C_REJ) | ((frame_number) ? 0 : (1 << 7));
	frame->control = *a++;
	if ((frame->control & 7) == C_REJ) {
		return BADFRAME_CODE;
	}
	const byte header_bcc = *a++;
	if (header_bcc != (frame->address ^ frame->control)) {
#ifdef DATA_LINK_DEBUG_MODE
		fprintf(stderr,"  parse_frame_from_array(): error: header bcc: %d.\n",__LINE__);
#endif
		++g_header_bcc_error_counter;
		return BADFRAME_CODE;
	}
	frame->size = 0;

	/* Supervision frame */
	if (*a == FLAG) {
#ifdef DATA_LINK_DEBUG_MODE
		fprintf(stderr,"  parse_frame_from_array(): read a supervision frame.\n");
#endif
		return SUCCESS_CODE;
	}

	if (*(a + 1) == FLAG) {
#ifdef DATA_LINK_DEBUG_MODE
		fprintf(stderr,"  parse_frame_from_array(): error: only 1B remaining.\n");
#endif
		return BADFRAME_CODE;
	}

	/* Data frame */
	byte data_bcc = 0;
	size_t num_bytes = 0;
	while (1) {
		if (num_bytes > LL_MAX_PAYLOAD_STUFFED) {
#ifdef DATA_LINK_DEBUG_MODE
			fprintf(stderr,"  parse_frame_from_array(): line %d.\n",__LINE__);
#endif
			return BADFRAME_CODE;
		}
		if (frame->size > frame->max_data_size) {
#ifdef DATA_LINK_DEBUG_MODE
			fprintf(stderr,"  parse_frame_from_array(): line %d.\n",__LINE__);
#endif
			return BADFRAME_CODE;
		}

		/*
		 * Stop loop condition.
		 */
		if (a[num_bytes + 1] == FLAG) {
			if (a[num_bytes] != data_bcc) {
				++g_data_bcc_error_counter;
#ifdef DATA_LINK_DEBUG_MODE
				fprintf(stderr,"parse_frame_from_array(): data bcc: line %d.\n",__LINE__);
				fprintf(stderr, "frame size %ld\n", frame->size);
				fprintf(stderr, "data bcc = %x\n", data_bcc);
				fprintf(stderr, "a[num_bytes] = %x\n", a[num_bytes]);
#endif
				return BADFRAME_CODE;
			}
#ifdef DATA_LINK_DEBUG_MODE
			fprintf(stderr,"  parse_frame_from_array(): successful read.\n");
#endif
			return SUCCESS_CODE;
		}

		byte c;
		if (a[num_bytes] == BS_ESC) {
			fprintf(stderr, "----\n");
			// remove byte stuffing
			++num_bytes;
			c = BS_OCT ^ a[num_bytes];
		} else {
			c = a[num_bytes];
		}
		++num_bytes;
		data_bcc ^= c;
		frame->data[frame->size++] = c;
		fprintf(stderr, "%x\n", c);
	}
}

static byte *
copy_and_stuff_bytes(byte *dest, const byte *src, const size_t src_size,
		byte *src_bcc)
{
	int bcc = 0;
	for (int i = 0; i < src_size; ++i) {
		byte c = src[i];
		bcc ^= c;
		if (c == FLAG || c == BS_ESC) {
			*dest++ = BS_ESC;
			*dest++ = BS_OCT ^ c;
		} else {
			*dest++ = c;
		}
		fprintf(stderr, "%2x\n", c);
	}
	*src_bcc = bcc;
	fprintf(stderr, "size %ld\n", src_size);
	fprintf(stderr, "bcc %2x\n", bcc);
	return dest;
}

/** \brief Send any type of frame.
 *
 * Compose a g_buffer[] array from a Frame and send it to the serial port.
 *
 * @return ERROR_CODE or ERROR_SUCCESS.
 */
Return_e f_send_frame(const int fd, const struct frame frame)
{
#ifdef DATA_LINK_DEBUG_MODE
	fprintf(stderr,"f_send_frame(): beginning frame writing (C=0x%2x, %zu bytes)\n",
			frame.control,frame.size);
#endif

	byte *bp = g_buffer;

	*bp++ = FLAG;

	// write header
	*bp++ = frame.address;
	*bp++ = frame.control;
	*bp++ = frame.address ^ frame.control; // bcc

	// copy data
	if (frame.size > LL_MAX_PAYLOAD_UNSTUFFED) {
#ifdef DATA_LINK_DEBUG_MODE
		fprintf(stderr,"f_send_frame(): tried to send too big a frame \
                (%zu bytes)\n",frame.size);
#endif
		return ERROR_CODE;

	} else if (frame.size > 0) { // frame might be 0 if it is supervision
		byte data_bcc;
		bp = copy_and_stuff_bytes(bp, frame.data, frame.size, &data_bcc);
		*bp++ = data_bcc;
#ifdef DATA_LINK_DEBUG_MODE
		fprintf(stderr,"f_send_frame(): unstuffed data size %ld.\n",frame.size);
#endif
	}

	*bp++ = FLAG;

	if (serial_port_write(fd, g_buffer, bp - g_buffer) < 0) {
#ifdef DATA_LINK_DEBUG_MODE
		fprintf(stderr,"f_send_frame(): writting failed.\n");
#endif
		return ERROR_CODE;
	}
	++g_sent_frame_counter;
#ifdef DATA_LINK_DEBUG_MODE
	fprintf(stderr,"f_send_frame(): finished sending frame #%ld\n",
			g_sent_frame_counter);
#endif
	return SUCCESS_CODE;
}

void start_alarm(int s)
{
#ifdef DATA_LINK_DEBUG_MODE
	fprintf(stderr,"Setting alarm: %d sec.\n",s);
#endif
	signal(SIGALRM, set_timeout_alarm);  // TODO: put in init function
	g_timeout_alarm = 0;
	alarm(s);
}

/** 
 */
Return_e f_receive_frame(const int fd, struct frame* frame, const int timeout_s)
{
#ifdef DATA_LINK_DEBUG_MODE
	fprintf(stderr,"  f_receive_frame(): beginning frame reception.\n");
#endif

	const int using_timeout = (timeout_s > 0);

	if (using_timeout) {
		start_alarm(timeout_s);
	}
	while (1) {
		while (serial_port_last_byte() != FLAG) { // first flag
#ifdef DATA_LINK_DEBUG_MODE
		fprintf(stderr,"  f_receive_frame(): looking for next flag.\n");
#endif

			if (serial_port_read(fd, g_buffer, FLAG, LL_MAX_FRAME_SZ) < 0) {
				return ERROR_CODE;
			}
			if (using_timeout && g_timeout_alarm) {
				return TIMEOUT_CODE;
			}

#ifdef DATA_LINK_DEBUG_MODE
			fprintf(stderr,"  f_receive_frame(): last byte=%x.\n",
					serial_port_last_byte());
#endif
		}
#ifdef DATA_LINK_DEBUG_MODE
		fprintf(stderr,"  f_receive_frame(): First FLAG detected.\n");
#endif
		g_buffer[0] = FLAG;

		// skip initial flags and read
		while (1) {
			if (using_timeout) {
				start_alarm(timeout_s);
			}
			int ret = serial_port_read(fd, g_buffer + 1, FLAG,
					LL_MAX_FRAME_SZ - 1);
			if (using_timeout && g_timeout_alarm) {
				return TIMEOUT_CODE;
			}
			if (ret < 0) {
				fprintf(stderr, "  f_receive_frame(): error.\n");
				return -1;
			}
			if (ret > 1) {
				break;
			}
		}

		if (serial_port_last_byte() == FLAG) { // final flag
#ifdef DATA_LINK_DEBUG_MODE
		fprintf(stderr,"  f_receive_frame(): Last FLAG detected.\n");
#endif
			Return_e ret = parse_frame_from_array(frame, g_buffer);
			if (ret == SUCCESS_CODE) {
				++g_rec_frame_counter;
			}
			return ret;
		}
		if (using_timeout && g_timeout_alarm) {
			return TIMEOUT_CODE;
		}
	}
}

/** Sends 'frame' and gets reply. */
int f_send_acknowledged_frame(const int fd, const unsigned num_retransmissions,
		const int timeout_s, struct frame out_frame, struct frame *reply)
{
	int ntries = (num_retransmissions <= 0) ? -1 : num_retransmissions;

	while (ntries > 0) {
#ifdef DATA_LINK_DEBUG_MODE
		fprintf(stderr,"f_send_acknowledged_frame(): ntries = %d.\n",ntries);
#endif

		if (f_send_frame(fd, out_frame) == ERROR_CODE) {
#ifdef DATA_LINK_DEBUG_MODE
			fprintf(stderr,"f_send_acknowledged_frame(): error writting\n");
#endif
			return -1;
		}

		reply->control = 0;
		Return_e ret = f_receive_frame(fd, reply, timeout_s);

		if (ret == ERROR_CODE) {
#ifdef DATA_LINK_DEBUG_MODE
			fprintf(stderr,"f_send_acknowledged_frame(): error reading\n");
#endif
			return -1;
		} else if (ret == TIMEOUT_CODE) {
#ifdef DATA_LINK_DEBUG_MODE
			fprintf(stderr,"f_send_acknowledged_frame(): timeout\n");
#endif
			--ntries;
			continue;
		} else if (ret == BADFRAME_CODE) {     // reset number of attempts
#ifdef DATA_LINK_DEBUG_MODE
				fprintf(stderr,"f_send_acknowledged_frame(): bad frame\n");
#endif
			ntries--;
			continue;
		} else {
			break;
		}
	}

	return ntries;
}
