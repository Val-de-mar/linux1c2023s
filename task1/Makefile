obj-m += chardev.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	gcc tester.c userspace_api.c -o tester.out

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

