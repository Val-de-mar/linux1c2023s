obj-m += custom_fifo.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	gcc -o reader reader.c
	gcc -o writer writer.c
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
