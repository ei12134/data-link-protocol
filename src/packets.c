#include <stdio.h>
#include <string.h>
#include <time.h>
#include "data_link.h"
#include "byte.h"
#include "packets.h"

#define MAX_FD 10
#define PACKET_SIZE LL_MAX_PAYLOAD_UNSTUFFED
#define NUM_FRAMES_PER_CALL 10
#define TIMEOUT_S 10
#define MICRO_TIMEOUT_DS 5
#define NUM_RETRANSMISSIONS 3
#define CLOSE_WAIT_TIME 2

int TRANSMITTER;

struct Connection g_connections[MAX_FD];

int llopen(char *port,int transmitter) 
{
    struct Connection conn;
    strcpy(conn.port,port);
    conn.packet_size = PACKET_SIZE;
    conn.micro_timeout_ds = MICRO_TIMEOUT_DS;
    conn.timeout_s = TIMEOUT_S;
    conn.num_retransmissions = NUM_RETRANSMISSIONS;
    conn.close_wait_time = CLOSE_WAIT_TIME;

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

void print_status(time_t t0,size_t num_bytes,unsigned long counter)
{
    double dt = difftime(time(NULL),t0);
    double speed = ((double)(num_bytes*8))/dt;
    fprintf(stderr,"----------------------\n");
    fprintf(stderr,"Link layer transmission %ld: %lf bit per sec; %ldB of data\n",
		    counter,speed,num_bytes);
    fprintf(stderr,"----------------------\n");
}

// TODO: return values
int llwrite(const int fd,const char *buffer,int length)
{
    struct Connection *conn = &g_connections[fd];

    unsigned long counter = 0;

    byte* p = (byte*)buffer;
    while (length > 0) {
        time_t t = time(NULL);


	size_t remainder = length % conn->packet_size;
	size_t num_bytes = remainder == 0 ? conn->packet_size : remainder;
	//unsigned long num_bytes = ((length-1) % conn->packet_size) + 1;

        if (transmitter_write(conn,p,num_bytes) < 0) {
            return -1;
        }
	counter++;
        p += num_bytes;
        length -= num_bytes;

        print_status(t,num_bytes,counter);
    }
    return 0;
}

int llread(const int fd,const char *buffer,int buff_remaining)
{
    struct Connection* conn = &g_connections[fd];

    unsigned long counter = 0;

    byte* p = (byte*)buffer;
    while (conn->is_active && buff_remaining > 0) {
        time_t t = time(NULL);

        int num_bytes;
        if ((num_bytes = receiver_read(conn,p,buff_remaining,
                        NUM_FRAMES_PER_CALL)) < 0) {
            fprintf(stderr,"llread(): there was an error\n");
            break;
        }
        if (num_bytes > buff_remaining) {
            fprintf(stderr,"too many bytes written... %d\n",__LINE__);
        }
	counter++;
        p += num_bytes;
        buff_remaining -= num_bytes;

        print_status(t,num_bytes,counter);
    }
    return p - (byte*)buffer;
}

int llclose(int fd)
{
    return disconnect(&g_connections[fd]);
}
