/* 
 * chardev.c - Create an input/output character device 
 */ 
 
#include <linux/atomic.h>
#include <linux/string.h>
#include <linux/cdev.h> 
#include <linux/delay.h> 
#include <linux/device.h> 
#include <linux/fs.h> 
#include <linux/init.h> 
#include <linux/module.h> /* Specifically, a module */ 
#include <linux/printk.h> 
#include <linux/types.h> 
#include <linux/uaccess.h> /* for get_user and put_user */ 
 
#include <asm/errno.h> 
 
#include "chardev.h" 
#define SUCCESS 0 
#define DEVICE_NAME "char_dev" 
#define BUF_LEN 80 
 
enum { 
    CDEV_NOT_USED = 0, 
    CDEV_EXCLUSIVE_OPEN = 1, 
}; 



/* Is the device open right now? Used to prevent concurrent access into 
 * the same device 
 */ 
static atomic_t already_open = ATOMIC_INIT(CDEV_NOT_USED); 

static struct StoreFormat* user_data_p = 0;
static __u32 limit = 0;
static __u32 allocated = 0;
static __u32 free = 0;


static struct class *cls;

/* This is called whenever a process attempts to open the device file */
static int device_open(struct inode *inode, struct file *file)
{
    pr_info("device_open(%p)\n", file);

    try_module_get(THIS_MODULE);
    return SUCCESS;
}

static int device_release(struct inode *inode, struct file *file)
{
    pr_info("device_release(%p,%p)\n", inode, file);

    module_put(THIS_MODULE);
    return SUCCESS;
}

static long add_into_list(unsigned long ioctl_param) {
    struct StoreFormat* place = user_data_p;
    if (free != 0) {
        u32 shift = 0;
        --free;
        for(;! place[shift].tombstone; ++shift) {}
        place += shift;
        place->tombstone = 0;
        printk("inside read\n");
        if (copy_from_user(&(place->data), (void __user *) ioctl_param, sizeof(struct UserPersonalData))) {
            place->tombstone = 1;
            ++free;
            return -EFAULT;
        }
    } else if (limit <  allocated) {
        place += limit;
        ++limit;
        printk("inside read\n");
        if (copy_from_user(&(place->data), (void __user *) ioctl_param, sizeof(struct UserPersonalData))) {
            --limit;
            return -EFAULT;
        }
    } else {

        printk("inside realloc\n");
        if (allocated == 0) {
            user_data_p = kcalloc(sizeof(struct StoreFormat), 10, GFP_KERNEL);
            if (user_data_p == 0) {
                return -EFAULT;
            }
            place = user_data_p;
            allocated = 10;
        } else {
            user_data_p = krealloc_array(user_data_p, sizeof(struct StoreFormat), 2 * allocated, GFP_KERNEL);
            if (user_data_p == 0) {
                return -EFAULT;
            }
            allocated *= 2;
            place = user_data_p;
        }
        place += limit;
        ++limit;
        if (copy_from_user(&(place->data), (void __user *) ioctl_param, sizeof(struct UserPersonalData))) {
            --limit;
            return -EFAULT;
        }
    }

    place->data.name[63] = '\0';
    place->data.surname[63] = '\0';
    place->data.ph_number[15] = '\0';
    return SUCCESS;
}

static long del_from_list(u32 place) {
    if (place >= limit || user_data_p[place].tombstone)
    {
        return -EEXIST;
    }
    user_data_p[place].tombstone = 1; // true
    if (place + 1 == limit) {
        --limit;
    }
    return SUCCESS;
}

static long find_by_surname(unsigned long ioctl_param) {
    static char buff[64];
    long place;

    if(copy_from_user(buff, ((void __user*)ioctl_param) + offsetof(struct DataRequest, surname), 64)) {
        return -2;
    }
    buff[63] = '\0';
    place= 0;
    for (;place < limit && (!user_data_p[place].tombstone) && strncmp(buff, user_data_p[place].data.surname, 64);++place);

    if (place == limit) return -1;

    return place;
}


static long
device_ioctl(struct file *file, /* ditto */
             unsigned int ioctl_num, /* number and param for ioctl */
             unsigned long ioctl_param)
{
    long ret = SUCCESS;

    printk("inside %u\n", ioctl_num);

    /*to avoid data race*/
    if (atomic_cmpxchg(&already_open, CDEV_NOT_USED, CDEV_EXCLUSIVE_OPEN))
        return -EBUSY;



    switch (ioctl_num) {
        case IOCTL_CHARDEV_READ_ENTRY: {
            long place;
            printk("inside read\n");
            place= find_by_surname(ioctl_param);

            if (place < 0) {
                ret = -EEXIST;
                break;
            }

            if(copy_to_user(
                    ((void __user*)ioctl_param) +offsetof(ReadRequest, data),
                    &(user_data_p[place].data),
                    sizeof(struct UserPersonalData)
            )) {
                ret = -EFAULT;
            }
            break;
        }
        case IOCTL_CHARDEV_ADD_ENTRY: {
            printk("inside add\n");
            ret = add_into_list(ioctl_param);
            break;
        }
        case IOCTL_CHARDEV_REMOVE_ENTRY: {
            long place;
            printk("inside remove\n");
            place = find_by_surname(ioctl_param);

            if (place == -1) {
                ret = -EEXIST;
                break;
            } else if (place < -1) {
                ret = -EFAULT;
                break;
            }

            ret = del_from_list(place);
            break;
        }
    }
 
    /* We're now ready for our next caller */ 
    atomic_set(&already_open, CDEV_NOT_USED); 
 
    return ret; 
} 
 

static struct file_operations fops = {
    .unlocked_ioctl = device_ioctl,
    .open = device_open,
    .release = device_release,
};

static int __init chardev2_init(void)
{
    /* Register the character device (at least try) */
    int ret_val = register_chrdev(MAJOR_NUM, DEVICE_NAME, &fops);

    /* Negative values signify an error */
    if (ret_val < 0) {
        pr_alert("%s failed with %d\n",
                 "Sorry, registering the character device ", ret_val);
        return ret_val;
    }

    cls = class_create(THIS_MODULE, DEVICE_FILE_NAME);
    device_create(cls, NULL, MKDEV(MAJOR_NUM, 0), NULL, DEVICE_FILE_NAME);

    pr_info("Device created on /dev/%s\n", DEVICE_FILE_NAME);

    return 0;
}
 
/* Cleanup - unregister the appropriate file from /proc */ 
static void __exit chardev2_exit(void) 
{ 
    device_destroy(cls, MKDEV(MAJOR_NUM, 0)); 
    class_destroy(cls); 
 
    /* Unregister the device */ 
    unregister_chrdev(MAJOR_NUM, DEVICE_NAME); 
} 
 
module_init(chardev2_init);
module_exit(chardev2_exit); 
 
MODULE_LICENSE("GPL");