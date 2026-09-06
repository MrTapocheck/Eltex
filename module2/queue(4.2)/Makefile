CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = queue
SOURCES = queue.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)

.PHONY: all clean