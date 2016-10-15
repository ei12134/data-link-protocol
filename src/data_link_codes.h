#define FLAG    0x7E    /* Header data types */
#define A       0x03    /* Address field */

/* Control field: */
#define C_SET   0x07    /* Set up */
#define C_DISC  0x0B    /* Disconnect */
#define C_UA    0x03    /* Unnumbered acknowledgment */
#define C_RR    0x01    /* Receiver ready / positive ACK */
#define C_REJ   0x07    /* Reject / negative ACK */

#define C_NBIT  0x05

/* Byte stuffing */
#define BS_ESC 0x7d
#define BS_OCT 0x20

