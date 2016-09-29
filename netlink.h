#ifndef NETLINK_H_
#define NETLINK_H_

#define FALSE 0
#define TRUE 1

#define RECEIVER 0
#define TRANSMITTER 1

/* Array indexes */
#define A_INDEX 0
#define C_INDEX 1
#define BCC1_INDEX 2

/* Header data types */
#define FLAG 0x7E

/* Address field */
#define A 0x03
#define A2 0x01

/* Control field */
#define C_SET 0x07  /* Set up */
#define C_DISC 0x0B /* Disconnect */
#define C_UA 0x03   /* Unnumbered acknowledgment */
#define C_RR 0x01   /* Receiver ready / positive ACK */
#define C_REJ 0x07  /* Reject / negative ACK */

#endif /* NETLINK_H_ */
