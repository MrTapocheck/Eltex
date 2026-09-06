CC = gcc
CFLAGS = -Wall -Wextra -std=c11
LDFLAGS = -ldl -lm

OPS = sum multiply max min divide
LIBS = $(OPS:%=libs/lib%.so)

.PHONY: all clean

all: calc $(LIBS)

calc: main.c needed.h
	$(CC) $(CFLAGS) main.c -o calc $(LDFLAGS)

libs/lib%.so: calc.c needed.h
	@mkdir -p libs
	$(CC) $(CFLAGS) -fPIC -shared -DBUILD_$* calc.c -o $@ $(LDFLAGS)

clean:
	rm -f calc
	rm -rf libs
