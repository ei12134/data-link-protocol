CC = gcc
PROG = netlink
CFLAGS = -Wall
SRCS = *.c layers/application/*.c layers/link/*.c
BIN_DIR = bin

all: bin netlink

bin:
	mkdir -p ${BIN_DIR}

netlink:
	$(CC) $(SRCS) -o $(BIN_DIR)/$(PROG) $(CFLAGS)	

clean:
	@rm -f $(BIN_DIR)/$(PROG) $(BIN_DIR)/*.o core

