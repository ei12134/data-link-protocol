#ifndef FRAMES_H_
#define FRAMES_H_

#include "byte.h"
#include <stddef.h>

struct Frame {
    byte address;
    byte control;
    size_t max_data_size;
    size_t size;
    byte *data;
};

typedef enum return_code_enum {
    SUCCESS_CODE = 0,
    ERROR_CODE = -1,
    TIMEOUT_CODE = -2,
    BADFRAME_CODE = -3
} Return_e;

struct Frame FRAME(const byte control);

#define SET (FRAME(C_SET))
#define DISC (FRAME(C_DISC))
#define UA (FRAME(C_UA))

/* Functions */
Return_e f_send_frame(const int fd,const struct Frame);
Return_e f_receive_frame(const int fd,struct Frame* frame,const int timeout_ds);
int f_send_acknowledged_frame(
        const int fd,
        const unsigned num_retransmissions,
        const int timeout_s,
        struct Frame out,
        struct Frame* in);
void f_print_frame(const struct Frame frame);
void f_dump_frame_buffer(const char *filename);

#endif // FRAMES_H_
