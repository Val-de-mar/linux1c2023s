#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

static char buf[8*4096 + 1];

int main() {
    int fd = open("/dev/custom_fifo", O_RDWR);
    if (fd < 0) {
        printf("cannot open\n");
        return 1;
    }

    int rd = read(fd, buf, 8*4096 - 15);
    buf[8*4096 - 15] = '\0';
    printf("read %d\n", rd);

    // printf("%s", buf);
    
    rd = read(fd, buf,  15);
    buf[15] = '\0';
    printf("read %d\n", rd);


    return 0;
}