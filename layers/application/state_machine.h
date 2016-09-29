#ifndef STATE_MACHINE_H_
#define STATE_MACHINE_H_

#define EXPECT_SET 0
#define EXPECT_UA 1
#define EXPECT_DISC 2
#define EXPECT_ACK 3
#define EXPECT_DATA 4

int sm_receiver(int fd);
int receive_set(int fd);
int sm_disc(int fd);
int sm_ua(int fd);

#endif // STATE_MACHINE_H_