CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = bintelephone_book
SOURCES = main.c sourcebintree.c

all: $(TARGET)

$(TARGET): $(SOURCES) needed.h
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET) contacts.dat

.PHONY: all clean