obj-m += mmaneg.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	gcc -o tests ./tests.c 

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
