CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=gnu11 -g
LDLIBS = -lrt

TARGET = chat

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $(TARGET) main.c $(LDLIBS)

clean:
	rm -f $(TARGET)