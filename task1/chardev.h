
#ifndef CHARDEV_H 
#define CHARDEV_H 
 
#include <linux/ioctl.h> 
#include "UserPersonalData.h"
 
/* The major device number. We can not rely on dynamic registration 
 * any more, because ioctls need to know it. 
 */ 
#define MAJOR_NUM 100 


#define IOCTL_CHARDEV_READ_ENTRY _IOWR(MAJOR_NUM, 0, ReadRequest*)
 
#define IOCTL_CHARDEV_ADD_ENTRY _IOWR(MAJOR_NUM, 1,  AddRequest*)
 
#define IOCTL_CHARDEV_REMOVE_ENTRY _IOWR(MAJOR_NUM, 2, RemoveRequest*)



 
/* The name of the device file */ 
#define DEVICE_FILE_NAME "char_dev" 
#define DEVICE_PATH "/dev/char_dev" 
 
#endif