#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

static char buf[8*4096];

int main() {
    int fd = open("/dev/custom_fifo", O_RDWR);
    if (fd < 0) {
        printf("cannot open\n");
        return 1;
    }
    memset(buf, 'a', 8*4096);
    int written = write(fd, buf, 8*4096);
    printf("written %d\n", written);
    return 0;
}