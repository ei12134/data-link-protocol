#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "../link/serial_port.h"
#include "../link/state_machine.h"
#include "transmitter.h"

int alarm_flag = TRUE;

// Set alarm_flag
void answer_alarm() { alarm_flag = TRUE; }

int transmitter(char *serial_port) {

  char abort = FALSE;
  int state = 0, retCode = 0;

  // Sends signal to SIGALARM
  (void)signal(SIGALRM, answer_alarm);

  // Initialize serial port session
  int fd = llopen(serial_port, 30);

  while (!abort) {

    switch (state) {

    // send SET
    case 0:
      printf("\n* State 0: Send SET *\n");
      retCode = send(fd, SUPERVISION, C_SET, TRUE, TRUE);

      switch (retCode) {
      case 0:
        state++;
        break;
      case 3:
        abort = TRUE;
        break;
      default:
        break;
      }
      break;

    // send DISCONNECT
    case 1:
      printf("\n* State 1: Send DISC *\n");
      retCode = send(fd, SUPERVISION, C_DISC, TRUE, TRUE);

      switch (retCode) {
      case 0:
        state++;
        break;
      case 3:
        abort = TRUE;
        break;
      default:
        break;
      }
      break;

    // send UA
    case 2:
      printf("\n* State 2: Send UA *\n");
      retCode = send(fd, SUPERVISION, C_UA, FALSE, FALSE);
      abort = TRUE;
      break;

      break;
    default:
      break;
    }
  }

  // End serial port session
  llclose(fd);
}

int send(int fd, char *frame, int num_retry, char expected_reply_type) {

  int counter = 0;
  int ret = 0;

  if (protected == TRUE) {
    while (counter < MAX_TRANSMITTER_ATTEMPTS) {
      if (alarm_flag) {
        if (ret = send_supervision(fd, frame, expected_reply_type))
          return ret;

        // Retry
        counter++;
        alarm_flag = FALSE;
        alarm(RETRY_TIMEOUT);
      }
    }
    if (counter >= MAX_TRANSMITTER_ATTEMPTS) {
      printf("Maximum retransmission attempts reached\n");
      return 3;
    }
  } else {
    send_supervision(fd, frame, expected_reply_type);
  }

  return ret;
}

int send_frame(int fd, char *frame, char expected_reply_type) {
  // Send message
  llwrite(fd, frame, strlen(frame));

  switch (expected_reply_type) {

  case EXPECT_SET:
    break;
  case EXPECT_UA:
    break;
  case EXPECT_DISC:
    break;
  case EXPECT_ACK:
    break;
  case EXPECT_DATA:
    break;
  }

  if (expect_reply) {
    // Get reply message
    int ret = 0;

    // Error
    else {
      printf("Error: receiver did not respond in time\n");
      printf("Waiting %d seconds before making a new attempt\n\n",
             RETRY_TIMEOUT);
      return 2;
    }
  } else
    return 0;
}
