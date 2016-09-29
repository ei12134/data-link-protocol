#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "serial_port.h"
#include "../../utils/byte_stuffing.h"

struct termios oldtio;

char set[3] = {A, C_SET, A ^ C_SET};
char ua[3] = {A, C_UA, A ^ C_UA};
char disc[3] = {A, C_DISC, A ^ C_DISC};

// Function to set header on a message
void build_header(char *msg, char control_field) {
  // msg[0] = FLAG;
  msg[0] = A;
  msg[1] = control_field;
  msg[2] = A ^ control_field; // compute_bcc(&msg[1], 2);
                              // msg[4] = FLAG;
}

// Function to compute the first bcc
char compute_bcc(char *msg, int length) {
  char bcc = msg[0];
  int i;
  for (i = 1; i < length; i++)
    bcc ^= msg[i];

  return bcc;
}

// Function that returns the packet type
char *packet_type(const char *packet) {
  char *type;

  // printf("SET compare: %s\n" ,packet_content(set, 3));

  if ((strncmp(packet, set, 3) == 0))
    type = "SET";
  else if ((strncmp(packet, ua, 3) == 0))
    type = "UA";
  else if ((strncmp(packet, disc, 3) == 0))
    type = "DISC";
  else
    type = "Unknown packet type";

  return type;
}

// Function to print the pack content
char *packet_content(const char *packet, const int size) {
  const char *hex = "0123456789ABCDEF";
  char *content = (char *)malloc(sizeof(char) * (3 * size));
  char *pout = content;
  const char *pin = packet;

  int i;

  if (pout) {

    for (i = 0; i < size - 1; i++) {
      *pout++ = hex[(*pin >> 4) & 0xF];
      *pout++ = hex[(*pin++) & 0xF];
      *pout++ = ':';
    }
    *pout++ = hex[(*pin >> 4) & 0xF];
    *pout++ = hex[(*pin++) & 0xF];
    *pout = 0;
  }

  return content;
}

// Function to read a message
int llread(int fd, char *buffer) {
  int i = 0;
  int res = 0;
  int start_flag = FALSE;
  char c;
  // printf("\n. . . Listening . . .\n\n");

  while (TRUE) {
    res = read(fd, &c, 1);

    if (res > 0) {
      if ((c == FLAG && start_flag) || i >= BUFSIZE)
        break;
      else if (c == FLAG)
        start_flag = TRUE;

      else {
        buffer[i] = c;
        i++;
      }
    } else
      return -1;
  }

  int new_length = destuff_bytes(buffer, i);

  if (compute_bcc(&buffer[A_INDEX], 2) != buffer[BCC1_INDEX])
    return -3;

  if (i > 0) {
    printf("Received packet: [type %s] [content %s] [bcc 0x%02X]\n",
           packet_type(buffer), packet_content(buffer, new_length),
           buffer[BCC1_INDEX]);
  }

  return new_length;
}

// Function to write a message
void llwrite(int fd, char *buffer, int length) {
  char *out;
  int new_length = stuff_bytes(buffer, length, &out);

  printf("out: %s\n", packet_content(out, new_length));

  char f[1] = {FLAG};

  write(fd, f, 1);
  int res = write(fd, out, new_length);
  write(fd, f, 1);

  printf("Sent packet: [type %s] [content %s] [%d byte(s)]\n",
         packet_type(buffer), packet_content(buffer, length), res);

  free(out);
}

// Function to open serial port
int llopen(char *serial_port, char timeout) //, int mode RECEIVER | TRANSMITTER?
{
  /*
  Open serial port device for reading and writing and not as controlling
  tty because we don't want to get killed if linenoise sends CTRL-C.
  */
  int fd = -1;
  struct termios newtio;

  fd = open(serial_port, O_RDWR | O_NOCTTY);

  if (fd < 0) {
    perror(serial_port);
    return -1;
  }

  if (tcgetattr(fd, &oldtio) == -1) { /* save current port settings */
    perror("tcgetattr");
    return -1;
  }

  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = OPOST;

  /* set input mode (non-canonical, no echo,...) */
  newtio.c_lflag &= ~(ICANON | ECHO);

  /* inter-character timer unused */
  newtio.c_cc[VTIME] = timeout;

  /* blocking read until 1 character is received */
  if (timeout == 0)
    newtio.c_cc[VMIN] = 1;
  else
    newtio.c_cc[VMIN] = 0;

  tcflush(fd, TCIFLUSH);

  if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
    perror("tcsetattr");
    return -1;
  }

  return fd;
}

// Function to close session
int llclose(int fd) {
  sleep(1);
  if (tcsetattr(fd, TCSANOW, &oldtio) == -1) {
    return -1;
  }
  close(fd);

  return 0;
}
