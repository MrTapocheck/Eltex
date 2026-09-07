CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic

all: producer_posix consumer_posix

producer_posix: producer_posix.c common_posix.c
	$(CC) -o $@ $^ $(CFLAGS)

consumer_posix: consumer_posix.c common_posix.c
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f producer_posix consumer_posix

.PHONY: all clean