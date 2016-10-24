#ifndef SERIAL_PORT_H_
#define SERIAL_PORT_H_

#include "byte.h"
extern int serial_port_baudrate;

byte serial_port_last_byte();
byte serial_port_previous_last_byte();
int serial_port_buffer_overflow();
int serial_port_open(const char *dev_name, const int micro_timeout);
int serial_port_close(int fd, int close_wait_time);
int serial_port_write(int fd, byte *data, int len);
int serial_port_read(int fd, byte *data, byte delim, int maxc);

#endif // SERIAL_PORT_H_
