#include "data_link_codes.h"
#include "data_link_io.h"
#include "data_link.h"
#include "serial_port.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <termios.h> // Baudrate

#define DEVICE "/dev/ttyS0"

int TRANSMITTER = 0;

struct Connection CONNECTION = {
    .max_buffer_size = LL_MAX_PAYLOAD,
    .num_retransmissions = 3,
    .baudrate = B300,
    .timeout_s = 3,
    .micro_timeout_ds = 11,
    .close_wait_time = 3
};

int
are_frames_equal(struct Frame f1,struct Frame f2)
{
    if (f1.address != f2.address) {
        fprintf(stderr,"are_frames_equal(): %d\n",__LINE__);
        return 0;
    }
    if (f1.control != f2.control) {
        fprintf(stderr,"are_frames_equal(): %d\n",__LINE__);
        return 0;
    }
    if (f1.size != f2.size) {
        fprintf(stderr,"are_frames_equal(): %d\n",__LINE__);
        return 0;
    }
    for (size_t i = 0; i < f1.size; i++) {
        if (f1.data[i] != f2.data[i]) {
            fprintf(stderr,"are_frames_equal(): %d\n",__LINE__);
            return 0;
        }
    }
    return 1;
}

// Print how the arguments must be
void print_help(char **argv)
{
	printf("Usage: %s [OPTION] <serial port>\n", argv[0]);
	printf("\n Program options:\n");
	printf("  -t         transmit data over the serial port\n");
}

// Verifies serial port argument
int parse_serial_port_arg(int index, char **argv)
{
    char *dev = argv[index];
	if ( (strcmp("/dev/ttyS0", dev)!= 0) &&
		(strcmp("/dev/ttyS1", dev)!=0) && 
		(strcmp("/dev/ttyS4", dev)!=0) ) {
		return -3;
    }
	return index;
} 

// Verifies arguments
int parse_args(int argc, char **argv)
{
	if (argc < 2)
		return -1;
		
	if (argc == 2)
		return parse_serial_port_arg(1, argv);
		
	if (argc == 3) {
		if ( (strcmp("-t", argv[1]) != 0) )
			return -2;
		else TRANSMITTER = 1;
		
		return parse_serial_port_arg(2, argv);
	}
	else return -1;
}

int test_1(struct Connection* conn)
{
    if (TRANSMITTER)
    {
        f_print_frame(SET);
        if (f_send_frame(conn->fd,SET) != SUCCESS_CODE) {
            f_dump_frame_buffer("FRAME");
            printf("line: %d\n",__LINE__);
            return 1;
        }

    } else {

        struct Frame frame;
        Return_e ret = f_receive_frame(conn->fd,&frame,0);
        f_print_frame(frame);
        f_print_frame(SET);
        f_dump_frame_buffer("FRAME");

        if (ret != SUCCESS_CODE) {
            printf("ret: %d\n",(int)ret);
            printf("line: %d\n",__LINE__);
            return 1;
        }
        if (!are_frames_equal(frame,SET)) {
            printf("line: %d\n",__LINE__);
            return 1;
        }
    }
    return 0;
}

int test_2(struct Connection* conn)
{
    if (TRANSMITTER)
    {
        sleep(1);

        f_print_frame(SET);
        struct Frame reply;
        if (0 > f_send_acknowledged_frame(conn->fd, 1, 10,SET,&reply)) {
            printf("line: %d\n",__LINE__);
            return -1;
        }
        if (reply.control != C_UA) {
            f_dump_frame_buffer("FRAME");
            printf("line: %d\n",__LINE__);
            return -1;
        }
    }
    else
    {
        struct Frame reply;
        Return_e ret = f_receive_frame(conn->fd,&reply,30);
        if (ret == ERROR_CODE) {
            fprintf(stderr,"ret = %d\n",ret);
            printf("line: %d\n",__LINE__);
            return 1;
        }
        if (reply.control == C_SET) {
            f_send_frame(conn->fd,UA);
        }
    }
    return 0;
}

int test_3(struct Connection* conn)
{
    int test_timeout_time = 3;

    if (TRANSMITTER)
    {
        f_print_frame(SET);
        f_dump_frame_buffer("FRAME");
        struct Frame reply;
        if (0 > f_send_acknowledged_frame(
                    conn->fd,
                    3,
                    test_timeout_time,
                    SET,
                    &reply)) {
            printf("line: %d\n",__LINE__);
            return 1;
        }
        if (reply.control != C_UA) {
            printf("line: %d\n",__LINE__);
            return 1;
        }
    }
    else
    {
        printf("Sleeping for %d seconds...",test_timeout_time+1);
        sleep(test_timeout_time+1); // force timeout

        struct Frame reply;
        Return_e ret;

        ret = f_receive_frame(conn->fd,&reply,3);
        if (ret == ERROR_CODE) {
            fprintf(stderr,"ret = %d\n",ret);
            printf("line: %d\n",__LINE__);
            return 1;
        }
        ret = f_receive_frame(conn->fd,&reply,3);
        if (ret == ERROR_CODE) {
            fprintf(stderr,"ret = %d\n",ret);
            printf("line: %d\n",__LINE__);
            return 1;
        }
        if (reply.control == C_SET) {
            f_send_frame(conn->fd,UA);
        }
    }
    return 0;
}

int test_4(struct Connection *conn)
{
    if (TRANSMITTER)
    {
        if (transmitter_connect(conn) < 0) {
            printf("line: %d\n",__LINE__);
            return 1;
        }
        printf("Connection established.\n");
        if (disconnect(conn) < 0) {
            printf("line: %d\n",__LINE__);
            return 1;
        }
    }
    else
    {
        if (receiver_listen(conn) < 0) {
            printf("line: %d\n",__LINE__);
            return 1;
        }
        if (wait_for_disconnect(conn,0) < 0) {
            printf("line: %d\n",__LINE__);
            return 1;
        }
    }
    return 0;
}

int test_single_message(struct Connection *conn,byte* data)
{
    if (TRANSMITTER)
    {
        if (transmitter_connect(conn) < 0) {
            printf("line: %d\n",__LINE__);
            return 1;
        }

        byte* s = data;
        if (transmitter_write(conn,s,strlen((char*)s)+1) < 0) {
            printf("line: %d\n",__LINE__);
            return 1;
        }
        printf("--- Transmitted: %s\n",(char*)s);

        if (disconnect(conn) < 0) {
            printf("line: %d\n",__LINE__);
            return 1;
        }
    }
    else
    {
        if (receiver_listen(conn) < 0) {
            printf("line: %d\n",__LINE__);
            return 1;
        }

        byte dest[8000];

        if (receiver_read(conn,dest,8000,0) < 0) {
            printf("line: %d\n",__LINE__);
            return 1;
        }
        printf("--- Received: %s\n",(char*)dest);
        if (strcmp((char*)dest,(char*)data) != 0) {
            printf("%s\n",(char*)dest);
            printf("%s\n",(char*)data);
            printf("line: %d\n",__LINE__);
            return 1;
        }
    }
    return 0;
}

int test_5(struct Connection* conn) 
{
    char *data = "isto é um teste";
    return test_single_message(conn,(byte*)data);
}

int test_6(struct Connection* conn) 
{
    char data[] = "Flag: x, Escape: x";
    data[6] = FLAG;
    data[17] = BS_ESC;
    return test_single_message(conn,(byte*)data);
}

int send_message(struct Connection* conn,byte* s)
{
    int len = strlen((char*)s);
    if (len == 0) {
        len = 1;
    }
    if (transmitter_write(conn,s,len) < 0) {
        printf("line: %d\n",__LINE__);
        return 1;
    }
    printf("--- Transmitted: %s\n",(char*)s);
    return 0;
}

int test_7(struct Connection* conn)
{
    char data1[] = "isto ";
    char data2[] = "é ";
    char data3[] = "um ";
    char data4[] = "teste ";
    char data5[] = "com ";
    char data6[] = "várias ";
    char data7[] = "tramas.";
    char data8[] = "";
    char final_string[] = "isto é um teste com várias tramas.";

    printf("data1 = %s\n",data1);

    if (TRANSMITTER)
    {
        if (transmitter_connect(conn) < 0) {
            printf("line: %d\n",__LINE__);
            return 1;
        }
        send_message(conn,(byte*)data1);
        send_message(conn,(byte*)data2);
        send_message(conn,(byte*)data3);
        send_message(conn,(byte*)data4);
        send_message(conn,(byte*)data5);
        send_message(conn,(byte*)data6);
        send_message(conn,(byte*)data7);
        send_message(conn,(byte*)data8);

        if (disconnect(conn) < 0) {
            printf("line: %d\n",__LINE__);
            return 1;
        }
    }
    else
    {
        if (receiver_listen(conn) < 0) {
            printf("line: %d\n",__LINE__);
            return 1;
        }

        byte dest[8000];

        if (receiver_read(conn,dest,8000,0) < 0) {
            printf("line: %d\n",__LINE__);
            return 1;
        }
        printf("--- Received: %s\n",(char*)dest);
        if (strcmp((char*)dest,final_string) != 0) {
            printf("%s\n",(char*)dest);
            printf("%s\n",final_string);
            printf("line: %d\n",__LINE__);
            return 1;
        }
    }
    return 0;
}

int main(int argc,char *argv[])
{
    int i;
	if ((i = parse_args(argc, argv)) < 0 ) {
		print_help(argv);
        printf("line: %d\n",__LINE__);
		return 1;
	}

    struct Connection conn = CONNECTION;

    strcpy(conn.port,argv[i]);

    if ((conn.fd = serial_port_open(conn.port, conn.micro_timeout_ds)) < 0) {
        printf("line: %d\n",__LINE__);
        return 1;
    }
    assert(test_1(&conn) == 0);
    printf(" ------------ Passed test 1\n");
    assert(test_2(&conn) == 0);
    printf(" ------------ Passed test 2\n");
    assert(test_3(&conn) == 0);
    printf(" ------------ Passed test 3\n");
    if (serial_port_close(conn.fd,3) < 0) {
        printf("line: %d\n",__LINE__);
        return 1;
    }

    assert(test_4(&conn) == 0);
    printf(" ------------ Passed test 4\n");
    assert(test_5(&conn) == 0);
    printf(" ------------ Passed test 5\n");
    assert(test_6(&conn) == 0);
    printf(" ------------ Passed test 6\n");
    assert(test_7(&conn) == 0);
    printf(" ------------ Passed test 7\n");
    printf("Finished\n");
    return 0;
}
