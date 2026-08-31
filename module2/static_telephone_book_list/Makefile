CFLAGS = -Wall -std=c11

telephone_book: main.c needed.h libs/libtelephone.a
	gcc $(CFLAGS) -I. main.c -Llibs -ltelephone -o telephone_book

libs/libtelephone.a: \
		libs/list_free.o \
		libs/find_node.o \
		libs/list_append.o \
		libs/source.o
	ar rcs libs/libtelephone.a \
		libs/list_free.o \
		libs/find_node.o \
		libs/list_append.o \
		libs/source.o

libs/list_free.o: libs/list_free.c needed.h libs/list_internal.h
	gcc $(CFLAGS) -I. -c libs/list_free.c -o libs/list_free.o

libs/find_node.o: libs/find_node.c needed.h libs/list_internal.h
	gcc $(CFLAGS) -I. -c libs/find_node.c -o libs/find_node.o

libs/list_append.o: libs/list_append.c needed.h libs/list_internal.h
	gcc $(CFLAGS) -I. -c libs/list_append.c -o libs/list_append.o

libs/source.o: libs/source.c needed.h libs/list_internal.h
	gcc $(CFLAGS) -I. -c libs/source.c -o libs/source.o

clean:
	rm -f telephone_book \
		libs/libtelephone.a \
		libs/list_free.o \
		libs/find_node.o \
		libs/list_append.o \
		libs/source.o
