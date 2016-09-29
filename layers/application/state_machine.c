#include <stdio.h>
#include "state_machine.h"
#include "../link/serial_port.h"

int sm_transmitter(int fd) {

  int state = 0;

  while (state < 3) {

    switch (state) {
    // send SET and reply UA
    case 0:
      printf("\n* State 0: Send SET & receive UA *\n");

      llwrite(fd, set, HEADER_SIZE);

      if (sm_ua(fd) == 0) {
        state++;
      }
      break;

    // DISCONNECT
    case 1:
      printf("\n* State 1: Receive DISCONNECT *\n");
      if (sm_disc(fd) == 0) {
        send_supervision(fd, C_DISC, TRUE);
        state++;
      }
      break;
    case 2:
      printf("\n* State 1: Receive UA *\n");
      if (sm_ua(fd) == 0) {
        state++;
      }
      break;

    default:
      break;
    }
  }
  return 0;
}

int sm_receiver(int fd) {

  int state = 0;

  while (state < 3) {

    switch (state) {
    // receive SET and reply UA
    case 0:
      printf("\n* State 0: Receive SET *\n");

      if (sm_set(fd) == 0) {
        send_supervision(fd, C_UA, TRUE);
        state++;
      }
      break;

    // DISCONNECT
    case 1:
      printf("\n* State 1: Receive DISCONNECT *\n");
      if (sm_disc(fd) == 0) {
        send_supervision(fd, C_DISC, TRUE);
        state++;
      }
      break;
    case 2:
      printf("\n* State 1: Receive UA *\n");
      if (sm_ua(fd) == 0) {
        state++;
      }
      break;

    default:
      break;
    }
  }
  return 0;
}

int receive_set(int fd) {
  char c;

  int state = 0;

  while (state != 5) {

    llread(fd, &c, 1);

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

// state machine for receiving disc message
int sm_disc(int fd) {
  char c;

  int state = 0;

  while (state != 5) {

    read(fd, &c, 1);

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
      else if (c == C_DISC)
        state = 3;
      else
        state = 0;
      break;
    case 3:
      if (c == FLAG)
        state = 1;
      else if (c == (A ^ C_DISC))
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

// state machine for receiving ua message
int sm_ua(int fd) {
  char c;

  int state = 0;

  while (state != 5) {

    read(fd, &c, 1);

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
      else if (c == C_UA)
        state = 3;
      else
        state = 0;
      break;
    case 3:
      if (c == FLAG)
        state = 1;
      else if (c == (A ^ C_UA))
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
