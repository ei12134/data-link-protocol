#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#include "serial_port.h"
#include "data_link.h"
#include "data_link_io.h"
#include "data_link_codes.h"
#include "byte.h"

volatile int g_timeout_alarm = 0;
void set_timeout_alarm() { g_timeout_alarm = 1; }

static byte g_buffer[LL_MAX_FRAME_SZ]; /** Local array for frame building. */
static long g_sent_frame_counter = 0;
static long g_rec_frame_counter = 0;
static long g_header_bcc_error_counter = 0;
static long g_data_bcc_error_counter = 0;

struct Frame FRAME(const byte control) {
    struct Frame super = { .address = A, .control = control, .size = 0 };
    return super;
}

void f_print_frame(const struct Frame frame)
{
    fprintf(stderr,"Frame:\n");
    fprintf(stderr,"A:%o C:%o S:%zu\n",
            frame.address,
            frame.control,
            frame.size);
    if (frame.size > 0) {
        for (int i=0; i < frame.size; i++) {
            putc(frame.data[i],stderr);
        }
        putc('\n',stderr);
    }
    putc('\n',stderr);
}

void f_dump_frame_buffer(const char *filename)
{
    FILE* f;
    if ((f=fopen(filename,"w")) == NULL) {
        fprintf(stderr,"f_dump_frame_buffer(): file error: line: %d\n",__LINE__);
    } else if (fprintf(f,"%.*s",LL_MAX_FRAME_SZ,g_buffer) < 0) {
        fprintf(stderr,"f_dump_frame_buffer(): file error: line: %d\n",
                __LINE__);
    } else if (fclose(f) == EOF) {
        fprintf(stderr,"f_dump_frame_buffer(): file error: line: %d\n",
                __LINE__);
    }
}

static Return_e
parse_frame_from_array(struct Frame* frame,byte *a)
{
    #ifdef DATA_LINK_DEBUG_MODE
    f_dump_frame_buffer("FRAME");
    fprintf(stderr,"parse_frame_from_array(): entering function.\n");
    #endif

    if (*a++ != FLAG) {
        #ifdef DATA_LINK_DEBUG_MODE
        fprintf(stderr,"parse_frame_from_array(): error: missing flag: \
                (line %d).\n",__LINE__);
        #endif
        return ERROR_CODE;
    }
    for (int i = 0; i <= 2; i++) { // the next fields should not have a FLAG
        if (a[i] == FLAG) {
            #ifdef DATA_LINK_DEBUG_MODE
            fprintf(stderr,"parse_frame_from_array(): error: unexpected flag \
                    (line %d).\n",__LINE__);
            #endif
            return BADFRAME_CODE;
        }
    }

    frame->address = *a++;
    // TODO: verificar que o 'control' é válido e sair antecipadamente
    frame->control = *a++;
    const byte header_bcc = *a++;
    if (header_bcc != (frame->address ^ frame->control)) {
        #ifdef DATA_LINK_DEBUG_MODE
        fprintf(stderr,"parse_frame_from_array(): header bcc: %d.\n",__LINE__);
        #endif
        ++g_header_bcc_error_counter;
        return BADFRAME_CODE;
    }
    frame->size = 0;

    if (a[0] == FLAG) {
        #ifdef DATA_LINK_DEBUG_MODE
        fprintf(stderr,"parse_frame_from_array(): read a supervision frame.\n");
        #endif
        return SUCCESS_CODE;
    }

    if (a[1] == FLAG) {
        #ifdef DATA_LINK_DEBUG_MODE
        fprintf(stderr,"parse_frame_from_array(): error: only 1B remaining.\n");
        #endif
        return BADFRAME_CODE;
    }

    byte bcc = 0;
    size_t num_bytes = 0;
    while (1) {
        if (num_bytes > LL_MAX_PAYLOAD || frame->size > frame->max_data_size) {
            #ifdef DATA_LINK_DEBUG_MODE
            fprintf(stderr,"parse_frame_from_array(): %d.\n",__LINE__);
            #endif
            return BADFRAME_CODE;
        }

        /*
         * Stop loop condition.
         */
        if (a[num_bytes+1] == FLAG) {
            if (a[num_bytes-1] == BS_ESC) {
                #ifdef DATA_LINK_DEBUG_MODE
                fprintf(stderr,"parse_frame_from_array(): %d.\n",__LINE__);
                #endif
                return BADFRAME_CODE;
            }
            if (a[num_bytes] != bcc) {
                ++g_data_bcc_error_counter;
                #ifdef DATA_LINK_DEBUG_MODE
                fprintf(stderr,"parse_frame_from_array(): data bcc: %d.\n",__LINE__);
                #endif
                return BADFRAME_CODE;
            }
            #ifdef DATA_LINK_DEBUG_MODE
            fprintf(stderr,"parse_frame_from_array(): successful read.\n");
            #endif
            return SUCCESS_CODE;
        }

        byte c;
        if (a[num_bytes] == BS_ESC) {
            // remove byte stuffing
            ++num_bytes;
            c = BS_OCT ^ a[num_bytes];
        } else {
            c = a[num_bytes];
        }
        ++num_bytes;
        bcc ^= c;
        frame->data[frame->size++] = c;
    }
}

static byte *
copy_and_stuff_bytes(
        byte *dest,
        const byte *data,
        const size_t size,
        byte *data_bcc)
{
    int bcc = 0;
    for (int i = 0; i < size; ++i) {
        byte c = data[i];
        bcc ^= c;
        if (c == FLAG || c == BS_ESC) {
            *dest++ = BS_ESC;
            *dest++ = BS_OCT^c;
        } else {
            *dest++ = c;
        }
    }
    *data_bcc = bcc;
    return dest;
}

/** \brief Send any type of frame.
 *
 * Compose a g_buffer[] array from a Frame and send it to the serial port.
 *
 * @return ERROR_CODE or ERROR_SUCCESS.
 */
Return_e
f_send_frame(const int fd,const struct Frame frame)
{
    #ifdef DATA_LINK_DEBUG_MODE
    fprintf(stderr,"f_send_frame(): beginning frame writing (C=%x, %zu B)\n",
            frame.control,frame.size);
    #endif

    byte *bp = g_buffer;

    *bp++ = FLAG;

    // write header
    *bp++ = frame.address;
    *bp++ = frame.control;
    *bp++ = frame.address ^ frame.control; // bcc

    // copy data
    if (frame.size > LL_MAX_FRAME_SZ) {
        #ifdef DATA_LINK_DEBUG_MODE
        fprintf(stderr,"f_send_frame(): tried to send too big a frame \
                (%zu bytes)\n",frame.size);
        #endif
        return ERROR_CODE;

    } else if (frame.size > 0) { // frame might be 0 if it is supervision
        byte data_bcc;
        bp = copy_and_stuff_bytes(bp,frame.data,frame.size,&data_bcc);
        *bp++ = data_bcc;
    }

    *bp++ = FLAG;

    if (serial_port_write(fd,g_buffer,bp - g_buffer) < 0) {
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

/** 
 */
Return_e
f_receive_frame(const int fd,struct Frame* frame,const int timeout_ds)
{
    #ifdef DATA_LINK_DEBUG_MODE
    fprintf(stderr,"f_receive_frame(): beginning frame reception.\n");
    #endif

    const int using_timeout = (timeout_ds > 0);
    if (using_timeout) {
        fprintf(stderr,"f_receive_frame(): setting timeout.\n");
        signal(SIGALRM,set_timeout_alarm);  // TODO: put in init function
        g_timeout_alarm = 0;
        alarm(timeout_ds);
    }

    while (1)
    {
        while (serial_port_last_byte() != FLAG) { // first flag
            #ifdef DATA_LINK_DEBUG_MODE
            fprintf(stderr,"f_receive_frame(): looking for next flag.\n");
            #endif

            if (serial_port_read(fd,g_buffer,FLAG,LL_MAX_FRAME_SZ) < 0) {
                return ERROR_CODE;
            }
            if (using_timeout && g_timeout_alarm) {
                return TIMEOUT_CODE;
            }

            #ifdef DATA_LINK_DEBUG_MODE
            fprintf(stderr,"f_receive_frame(): last byte=%x.\n",
                    serial_port_last_byte());
            #endif
        }
        #ifdef DATA_LINK_DEBUG_MODE
        fprintf(stderr,"f_receive_frame(): First FLAG detected.\n");
        #endif
        g_buffer[0] = FLAG;

        // skip initial flags and read
        while (1)
        {
            int ret = serial_port_read(fd,g_buffer+1,FLAG,LL_MAX_FRAME_SZ-1);
            if (using_timeout && g_timeout_alarm) {
                return TIMEOUT_CODE;
            }
            if (ret < 0) {
                fprintf(stderr,"f_receive_frame(): error.\n");
                return -1;
            }
            if (ret > 1){ 
                break;
            }
        }

        if (serial_port_last_byte() == FLAG) { // final flag
            #ifdef DATA_LINK_DEBUG_MODE
            fprintf(stderr,"f_receive_frame(): Last FLAG detected.\n");
            #endif
            Return_e ret = parse_frame_from_array(frame,g_buffer);
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
int
f_send_acknowledged_frame(
        const int fd,
        const unsigned num_retransmissions,
        const int timeout_s,
        struct Frame out_frame,
        struct Frame *reply)
{
    int ntries = (num_retransmissions <= 0) ? -1 : num_retransmissions;

    while (ntries > 0) {
        #ifdef DATA_LINK_DEBUG_MODE
        fprintf(stderr,"f_send_acknowledged_frame(): ntries = %d.\n",ntries);
        #endif

        if (f_send_frame(fd,out_frame) == ERROR_CODE) {
            #ifdef DATA_LINK_DEBUG_MODE
            fprintf(stderr,"f_send_acknowledged_frame(): error writting\n");
            #endif
            return -1;
        }

        reply->control = 0;
        Return_e ret = f_receive_frame(fd,reply,timeout_s);

        if (ret == ERROR_CODE) {
            #ifdef DATA_LINK_DEBUG_MODE
            fprintf(stderr,"f_send_acknowledged_frame(): error reading\n");
            #endif
            return -1;
        }
        else if (ret == TIMEOUT_CODE) {
            #ifdef DATA_LINK_DEBUG_MODE
            fprintf(stderr,"f_send_acknowledged_frame(): timeout\n");
            #endif
            --ntries;
            continue;
        }
        else if (ret == BADFRAME_CODE) {     // reset number of attempts
            #ifdef DATA_LINK_DEBUG_MODE
            fprintf(stderr,"f_send_acknowledged_frame(): bad frame\n");
            #endif
            ntries = num_retransmissions;
            continue;
        }
        else {
            break;
        }
    }

    return ntries;
}
