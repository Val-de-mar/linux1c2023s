#include "userspace_api.h"
#include "chardev.h"

#include <memory.h>
#include <unistd.h>
#include <fcntl.h>
#include <asm/errno.h>
#include <sys/ioctl.h>


long get_user(const char* surname, unsigned int len, struct UserPersonalData * output_data) {
    static ReadRequest rqst;
    unsigned int sz = len < 63?len:63;
    memcpy(rqst.surname, surname, sz);
    int fd = open("/dev/char_dev", O_RDWR);

    if (fd < 0) {
        return -EACCES;
    }

    int res = ioctl(fd, IOCTL_CHARDEV_READ_ENTRY, &rqst);
    close(fd);

    if (res) {
        return res;
    }

    memcpy(output_data, &(rqst.data), sizeof(struct UserPersonalData ));

    return 0;
}

long add_user(struct UserPersonalData* output_data) {
    int fd = open("/dev/char_dev", O_RDWR);

    if (fd < 0) {
        return -EACCES;
    }

    int res = ioctl(fd, IOCTL_CHARDEV_ADD_ENTRY, output_data);
    close(fd);

    return res;
}

long del_user(const char* surname, unsigned int len, struct UserPersonalData* output_data) {
    static RemoveRequest rqst;
    unsigned int sz = len < 63?len:63;
    memcpy(rqst.surname, surname, sz);
    int fd = open("/dev/char_dev", O_RDWR);

    if (fd < 0) {
        return -EACCES;
    }

    int res = ioctl(fd, IOCTL_CHARDEV_REMOVE_ENTRY, &rqst);
    close(fd);

    if (res) {
        return res;
    }

    if (output_data != 0) {
        memcpy(output_data, &(rqst.data), sizeof(struct UserPersonalData));
    }

    return 0;
}