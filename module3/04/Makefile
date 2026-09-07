CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic

all: producer consumer

producer: producer.c common.c
	$(CC) -o $@ $^ $(CFLAGS)

consumer: consumer.c common.c
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f producer consumer

.PHONY: all clean