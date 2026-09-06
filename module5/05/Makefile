obj-m += netlink_mod.o

all: module user

module:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

user: user.c constants.h
	gcc -Wextra -o user user.c

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f user

.PHONY: all module user clean