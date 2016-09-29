#ifndef SERIAL_PORT_H_
#define SERIAL_PORT_H_

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define BUFSIZE 4096
#define READ_TIMEOUT 70
#define RETRY_TIMEOUT 3
#define HEADER_SIZE 3
#define MAX_TRANSMITTER_ATTEMPTS 3

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
