CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = chmod
SOURCES = chmod.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)

.PHONY: all clean