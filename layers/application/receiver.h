#ifndef RECEIVER_H_
#define RECEIVER_H_

int receiver(char *serial_port);
int listen(int fd);
int listen_expecting(int fd, const char *expected);

#endif // RECEIVER_H_
