#include <stdio.h>
#include <string.h>
#include "state_machine.h"
#include "../link/serial_port.h"

int receive_set(int fd)
{
	char c[BUFSIZE] = { 0 };

	// int state = 0;

	llread(fd, c);
	printf("%s:%zu\n", c, strlen(c));

	// while (state != 5) {
	//   switch (state) {
	//   case 0:
	//     if (c == FLAG)
	//       state = 1;
	//     else
	//       state = 0;
	//     break;
	//   case 1:
	//     if (c == FLAG)
	//       state = 1;
	//     else if (c == A)
	//       state = 2;
	//     else
	//       state = 0;
	//     break;
	//   case 2:
	//     if (c == FLAG)
	//       state = 1;
	//     else if (c == C_SET)
	//       state = 3;
	//     else
	//       state = 0;
	//     break;
	//   case 3:
	//     if (c == FLAG)
	//       state = 1;
	//     else if (c == (A ^ C_SET))
	//       state = 4;
	//     else
	//       state = 0;
	//     break;
	//   case 4:
	//     if (c == FLAG)
	//       state = 5;
	//     else
	//       state = 0;
	//     break;
	//   default:
	//     state = 5;
	//     break;
	//   }
	// }
	return 0;
}
