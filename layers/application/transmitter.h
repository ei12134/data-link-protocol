#ifndef TRANSMITTER_H_
#define TRANSMITTER_H_

#define SUPERVISION 0
#define UNNUMBERED 1
#define INFORMATION 2

void answer_alarm();
int transmitter(char *serial_port);
int send(int fd, char *frame, int num_retry, char expected_reply_type);
int send_supervision(int fd, char control_field, int expect_reply);

#endif // TRANSMITTER_H_
