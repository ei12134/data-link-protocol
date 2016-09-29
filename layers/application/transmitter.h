#ifndef TRANSMITTER_H_
#define TRANSMITTER_H_

#include "../../netlink.h"

#define SUPERVISION 0
#define UNNUMBERED 1
#define INFORMATION 2

void answer_alarm();
int transmitter(char *serial_port);

#endif // TRANSMITTER_H_
