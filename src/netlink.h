#ifndef NETLINK_H_
#define NETLINK_H_

extern int serial_port_baudrate;

void help(char **argv);
int parse_serial_port_arg(int index, char **argv);
int parse_baudrate_arg(int baurdate_index, char **argv);
void parse_flags(int* t_index, int* i_index, int* b_index, int* f_index,
		int* r_index, int argc, char **argv);
int parse_args(int argc, char **argv, int *is_transmitter);
void receiver_stats();

#endif // NETLINK_H_
