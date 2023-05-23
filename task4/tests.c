#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>


char buffer[16*4096];

int main () {
    int fd = open("/proc/mmaneg", O_RDWR);
    if (fd < 0) {
        printf("cannot open /proc/mmaneg\n");
        return 1;
    }
    
    printf("file openned\ntesting listvma:\n");
    char * command = "listvma";
    write(fd, command, strlen(command));

    int sz = read(fd, buffer, sizeof(buffer) - 1);
    buffer[sz] = '\0';
    printf("%s\n\n", buffer);

    unsigned long long val = 0x415;

    command = buffer;
    printf("testing findpage:\n");
    snprintf(command, 16*4096, "findpage %llx", &val);
    write(fd, command, strlen(command));

    sz = read(fd, buffer, sizeof(buffer) - 1);
    if(sz <= 0) {
        printf("err, sz = %d\n", sz);
    }
    buffer[sz] = '\0';
    printf("ans:\n%s\n\n", buffer);

    unsigned long long newval = 0x154;
    printf("testing writewal:\n\ttrying to write %llx\n", newval);
    snprintf(command, 16*4096, "writeval %llx %lx", &val, newval);
    write(fd, command, strlen(command));

    printf("\tread %llx\n\n", val);

}