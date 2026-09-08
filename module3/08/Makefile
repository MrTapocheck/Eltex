CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic

all: udp_sniffer

udp_sniffer: udp_sniffer.c
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f udp_sniffer captured_udp.txt

.PHONY: all clean