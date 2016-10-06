#include "application.h"

struct spFrame create_supervision_frame(char address, char control,
		char protection)
{
	struct spFrame frame;

	frame.flags[0] = address;
	frame.flags[1] = control;
	frame.flags[2] = protection;

	return frame;
}

struct infoFrame create_information_frame(char address, char control,
		char protection1, char *data, char protection2)
{
	struct infoFrame frame;

	frame.prefix[0] = address;
	frame.prefix[1] = control;
	frame.prefix[2] = protection1;
	frame.data = data;
	frame.suffix[0] = protection2;

	return frame;
}

struct dataPacket create_data_packet(char control_field, char seq_num, char l1,
		char l2, char *data_packet)
{
	struct dataPacket packet;

	packet.control_field = control_field;
	packet.seq_num = seq_num;
	packet.l1 = l1;
	packet.l2 = l2;
	packet.data_packet = data_packet;

	return packet;
}

struct controlPacket create_control_packet(char control_field, char t1, char l1,
		char *v1, char t2, char l2, char *v2)
{
	struct controlPacket packet;

	packet.control_field = control_field;
	packet.t1 = t1;
	packet.l1 = l1;
	packet.v1 = v1;
	packet.t2 = t2;
	packet.l2 = l2;
	packet.v2 = v2;

	return packet;
}
