#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include "serial_port.h"
#include "data_link.h"
#include "byte.h"

#define FALSE 0
#define TRUE 1

#define BUFSIZE 4096

int TRANSMITTER = FALSE;

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
		else TRANSMITTER = TRUE;
		
		return parse_serial_port_arg(2, argv);
	}
	else return -1;
}

int test1(int argc,char **argv)
{
	// Verifies arguments
	int i = -1;
	if ( (i = parse_args(argc, argv)) < 0 ) {
		print_help(argv);
        printf("line: %d\n",__LINE__);
		return 1;
	}
	
    int timeout = 0;
    int fd = serial_port_open(argv[i],timeout);
    if (fd < 0) {
        fprintf(stderr,"serial_port_test: serial_port_open returned %d\n",fd);
        printf("line: %d\n",__LINE__);
        return 1;
    }

    byte s[BUFSIZE];
    if (TRANSMITTER) {
        byte s[BUFSIZE];
        sprintf((char*)s,"Um pequeno passo para o homem...");
        int len = strlen((char*)s);
        if (serial_port_write(fd,s,len+1) < 0) {
            printf("line: %d\n",__LINE__);
            return 1;
        }
        printf("Message sent: %s\n",s);

        for (int i=0; s[i] != '\0'; i++) {
            s[i] = 'x';
        }

        if (serial_port_read(fd,s,'\0',BUFSIZE) < 0) {
            printf("line: %d\n",__LINE__);
            return 1;
        }
        printf("Message received: %s\n",s);
    } else {
        if (serial_port_read(fd,s,'\0',BUFSIZE) < 0) {
            printf("line: %d\n",__LINE__);
            return 1;
        }
        printf("Message received: %s\n",s);

        int len = strlen((char*)s);
        if (serial_port_write(fd,s,len+1) < 0) {
            printf("line: %d\n",__LINE__);
            return 1;
        }
        printf("Message sent: %s\n",(char*)s);
    }

    if (serial_port_close(fd,3) < 0) {
        fprintf(stderr,"serial_port_test: serial_port_close returned \
                negative\n");
        printf("line: %d\n",__LINE__);
        return 1;
    }
    return 0;
}

int get_frame(byte *dest,byte *src,byte flag)
{
    for (int i = 0; i < BUFSIZE; i++) {
        if (src[i] == flag) {
            dest[i] = '\0';
            return i;
        }
        dest[i] = src[i];
    }
    return -1;
}

int test2(int argc,char **argv)
{
	// Verifies arguments
	int i = -1;
	if ( (i = parse_args(argc, argv)) < 0 ) {
		print_help(argv);
        printf("line: %d\n",__LINE__);
		return 1;
	}
	
    int timeout = 0;
    int fd = serial_port_open(argv[i],timeout);
    if (fd < 0) {
        fprintf(stderr,"serial_port_test: serial_port_open returned %d\n",fd);
        printf("line: %d\n",__LINE__);
        return 1;
    }


    if (TRANSMITTER) {
        byte *test_string = (byte*) "   F0001FF0002F0003F0004F  ";

        int len = strlen((char*)test_string);
        if (serial_port_write(fd,test_string,len+1) < 0) {
            printf("line: %d\n",__LINE__);
            return 1;
        }
    } else {
        // Read until first flag
        byte tmp[BUFSIZE];
        for (int i = 0; i < BUFSIZE; i++) { // init
            tmp[i] = 'x';
        }
        if (serial_port_read(fd,tmp,'F',BUFSIZE) < 0) {
            printf("line: %d\n",__LINE__);
            return 1;
        }

        byte dest[BUFSIZE];
        for (int i = 0; i < BUFSIZE; i++) { // init
            dest[i] = 'x';
        }

        byte *frames[] = 
        { (byte*)"0001", (byte*)"0002", (byte*)"0003", (byte*)"0004" };

        // Read a few frames
        for (int i = 0; i < 4; i++) {
            // Read a frame
            if (serial_port_read(fd,tmp,'F',BUFSIZE) < 0) {
                printf("line: %d\n",__LINE__);
                return 1;
            }

            byte plb = serial_port_previous_last_byte();
            printf("previous last byte: %c\n",plb);
            if (plb != 'F') {
                printf("line: %d\n",__LINE__);
                return 1;
            }

            int size = get_frame(dest,tmp,'F');
            if (size < 0) {
                printf("line: %d\n",__LINE__);
                return 1;
            } else if (size == 0) {
                // Space in between 'F's
                --i;
            } else {
                printf("frame %d: %s\n",i,(char*)dest);
                if (strcmp((char*)dest,(char*)frames[i]) != 0) {
                    printf("line: %d\n",__LINE__);
                    return 1;
                }
            }
        }
    }

    if (serial_port_close(fd,3) < 0) {
        fprintf(stderr,"serial_port_test: serial_port_close returned \
                negative\n");
        return 1;
    }

    return 0;
}

int main(int argc, char **argv)
{
    printf("Test 1...\n");
    assert(test1(argc,argv) == 0);

    printf("Test 2...\n");
    assert(test2(argc,argv) == 0);

	return 0;
}

