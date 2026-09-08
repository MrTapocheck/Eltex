CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic

all: server client

server: server.c
	$(CC) -o $@ $^ $(CFLAGS)

client: client.c
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f server client

.PHONY: all clean