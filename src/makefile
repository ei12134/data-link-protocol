CC = gcc
CFLAGS = -Wall -std=c99
#CFLAGS += -DSERIAL_PORT_DEBUG_MODE
CFLAGS += -DDATA_LINK_DEBUG_MODE

PROG = netlink serial_port_test writenoncanonical noncanonical writecanonical

all: $(PROG)

clean:
	@rm -f $(PROG) *.o core FRAME

ctags:
	@ctags . -R

serial_port_test: serial_port_test.o serial_port.o
	cc -o $@ $F $^

data_link_test: data_link_test.o data_link_io.o serial_port.o data_link.o
	cc -o $@ $F $^

netlink: netlink.o packets.o data_link.o data_link_io.o serial_port.o
	cc -o $@ $F $^

serial_port.o serial_port_test.o data_link_io.o data_link.o data_link_test.o: serial_port.h
data_link_io.o data_link.o data_link_test.o: data_link_io.h data_link_codes.h

data_link_io.o data_link.o serial_port.o data_link_test.io packets.o: byte.h

data_link_io.o data_link.o data_link_test.o packets.o: data_link.h

packets.o netlink.o: packets.h

writecanonical: ../example_code/writecanonical.c
writenoncanonical: ../example_code/writenoncanonical.c
noncanonical: ../example_code/noncanonical.c
canonical: ../example_code/canonical.c

.PHONY: clean
