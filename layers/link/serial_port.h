#ifndef SERIAL_PORT_H_
#define SERIAL_PORT_H_

#define FALSE 0
#define TRUE 1

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define BUFSIZE 4096
#define READ_TIMEOUT 70
#define RETRY_TIMEOUT 3
#define HEADER_SIZE 3
#define MAX_TRANSMITTER_ATTEMPTS 3

/* Receiver commands */

/* Transmitter commands */

/* Array indexes */
#define A_INDEX 0
#define C_INDEX 1
#define BCC1_INDEX 2

/* Header data types */
#define FLAG 0x7E

/* Address field */
#define A 0x03
#define A2 0x01

/* Control field */
#define C_SET 0x07  /* Set up */
#define C_DISC 0x0B /* Disconnect */
#define C_UA 0x03   /* Unnumbered acknowledgment */
#define C_RR 0x01   /* Receiver ready / positive ACK */
#define C_REJ 0x07  /* Reject / negative ACK */

extern char set[];
extern char ua[];
extern char disc[];

void build_header(char *msg, char control_field);
char compute_bcc(char *msg, int length);

char *packet_type(const char *buffer);
char *packet_content(const char *packet, const int size);

int llread(int fd, char *buffer);
void llwrite(int fd, char *buffer, int length);
int llopen(char *serial_port, char timeout);
int llclose(int fd);

#endif // SERIAL_PORT_H_
