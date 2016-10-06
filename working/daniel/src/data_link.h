#ifndef DATA_LINK_H_
#define DATA_LINK_H_

/* Settings */
#define LL_MAX_FRAME_SZ 4096
/* ---------------- */

// - 2 flags, 3-byte header, 1 data bcc
// '/2' because of worst case scenario byte stuffing
#define LL_MAX_PAYLOAD ((LL_MAX_FRAME_SZ-2-3-1)/2)

#include <stddef.h>
#include "byte.h"

struct Connection {
    int fd;
    unsigned long frame_number;

    /* Settings */
    char port[20];
    size_t max_buffer_size;
    size_t packet_size;
    unsigned num_retransmissions;
    unsigned baudrate;
    unsigned timeout_s;
    unsigned micro_timeout_ds;
    unsigned close_wait_time;
    int is_transmitter;
    int is_active;
};

int transmitter_connect(struct Connection* conn);
int transmitter_write(struct Connection* conn,byte* data,size_t size);

int receiver_listen(struct Connection* conn);
int receiver_read(struct Connection* conn,byte *data,size_t max_data_size,
        const int num_frames);

int disconnect(struct Connection *conn);
int wait_for_disconnect(struct Connection* conn,int timeout_s);

#endif // DATA_LINK_H_
