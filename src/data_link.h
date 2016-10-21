#ifndef DATA_LINK_H_
#define DATA_LINK_H_

/* Settings */
//#define LL_MAX_FRAME_SZ 128
//#define LL_MAX_FRAME_SZ 256
//#define LL_MAX_FRAME_SZ 512
//#define LL_MAX_FRAME_SZ 1024
//#define LL_MAX_FRAME_SZ 2048
#define LL_MAX_FRAME_SZ 4096
//#define LL_MAX_FRAME_SZ 8192
//#define LL_MAX_FRAME_SZ 16384
//#define LL_MAX_FRAME_SZ 32768
/* ---------------- */

// - 2 flags, 3-byte header, 1 data bcc
// '/2' because of worst case scenario byte stuffing
#define LL_MAX_PAYLOAD_STUFFED ((LL_MAX_FRAME_SZ-2-3-1)/2)
#define LL_MAX_PAYLOAD_UNSTUFFED (LL_MAX_PAYLOAD_STUFFED/2)

#include <stddef.h>
#include "byte.h"

struct connection {
	int fd;
	unsigned long frame_number;

	/* Settings */
	char port[20];
	size_t max_buffer_size;
	size_t frame_size;
	unsigned num_retransmissions;
	unsigned baudrate;
	unsigned timeout_s;
	unsigned micro_timeout_ds;
	unsigned close_wait_time;
	int is_transmitter;
	int is_active;
	size_t packet_size;
};

int transmitter_connect(struct connection* conn);
int transmitter_write(struct connection* conn, byte* data_packet, size_t size);

int receiver_listen(struct connection* conn);
int receiver_read(struct connection* conn, byte *data, size_t max_data_size,
		const int num_frames);

int disconnect(struct connection *conn);
int wait_for_disconnect(struct connection* conn, int timeout_s);

#endif // DATA_LINK_H_
