#include <stdio.h>
#include "state_machine.h"
#include "../link/serial_port.h"

int receive_set(int fd) {
  char c;

  int state = 0;

  while (state != 5) {

    llread(fd, &c);

    switch (state) {
    case 0:
      if (c == FLAG)
        state = 1;
      else
        state = 0;
      break;
    case 1:
      if (c == FLAG)
        state = 1;
      else if (c == A)
        state = 2;
      else
        state = 0;
      break;
    case 2:
      if (c == FLAG)
        state = 1;
      else if (c == C_SET)
        state = 3;
      else
        state = 0;
      break;
    case 3:
      if (c == FLAG)
        state = 1;
      else if (c == (A ^ C_SET))
        state = 4;
      else
        state = 0;
      break;
    case 4:
      if (c == FLAG)
        state = 5;
      else
        state = 0;
      break;
    default:
      state = 5;
      break;
    }
  }
  return 0;
}