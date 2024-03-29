#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "packets.h"

#define TRUE 1
#define FALSE 0

int TRANSMITTER = FALSE;
#define BUFSIZE (10*1024*1024)

// Print how the arguments must be
void print_help(char **argv)
{
	fprintf(stderr,"Usage: %s [OPTION] <serial port>\n", argv[0]);
	fprintf(stderr,"\n Program options:\n");
	fprintf(stderr,"  -t         transmit data over the serial port\n");
}

// Verifies serial port argument
int parse_serial_port_arg(int index, char **argv)
{
	if ( (strcmp("/dev/ttyS0", argv[index]) != 0) &&
		(strcmp("/dev/ttyS1", argv[index])!=0) && 
		(strcmp("/dev/ttyS4", argv[index])!=0) )
		return -3;
		
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

int main(int argc, char **argv)
{
	// Verifies arguments
	int i = -1;
	if ( (i = parse_args(argc, argv)) < 0 ) {
		print_help(argv);
		exit(1);
	}

    char *port = argv[i];

    int fd;
    if ((fd = llopen(port,TRANSMITTER)) < 0) {
        return 1;
    }
    char *buffer = malloc(sizeof(char)*BUFSIZE);
    int c;

    if (TRANSMITTER) {
        fprintf(stderr,"Transmitting...\n");
        do {
            int i = 0;
            while (i < BUFSIZE && (c = getc(stdin)) != EOF)  {
                buffer[i++] = c;
            }
            if (llwrite(fd,buffer,i) < 0) {
                fprintf(stderr,"netlink: closing prematurely\n");
                llclose(fd);
                return 1;
            }
        } while (c != EOF);
        return llclose(fd);
    }
    else {
        fprintf(stderr,"Receiving...\n");
        int num_bytes;
        do {
            if ((num_bytes = llread(fd,buffer,BUFSIZE)) < 0) {
                fprintf(stderr,"netlink: closing prematurely\n");
                llclose(fd);
                return 1;
            }
            /*
             * OUTPUT TO STDOUT!
             */
            printf("%.*s",num_bytes,buffer);
        } while (num_bytes > 0);
        return 0;
    }
}
