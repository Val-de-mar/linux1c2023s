#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <asm/page.h>

#define PROCFS_MAX_SIZE 1024
#define PROCFS_NAME "mmaneg"

static struct proc_dir_entry *mmaneg_entry;
static char procfs_buffer[PROCFS_MAX_SIZE];
static unsigned long procfs_buffer_size = 0;



enum CommandType {
    none,
    listvma,
    findpage,
    writeval
};

int command;
unsigned long address;


// Function to handle the "listvma" command
static void listvma_command(struct seq_file *m)
{
    struct vm_area_struct *vma;
    struct task_struct *task = current;

    // Traverse the VMAs of the current process
    seq_printf(m, "VMA list for PID %d:\n", task->pid);
    for (vma = task->mm->mmap; vma; vma = vma->vm_next) {
        seq_printf(m, "Start: 0x%lx, End: 0x%lx\n", vma->vm_start, vma->vm_end);
    }
}




// Function to handle the "findpage" command
static void findpage_command(struct seq_file *m, unsigned long addr)
{
    struct page* pg;

    int pg_num = get_user_pages_fast(addr, 1, 0, &pg);
    
    if (pg_num != 1) {
        seq_printf(m, "Translation not found for address 0x%lx\n", addr);
        return;
    }



    
	// spinlock_t *ptl;
    // pte_t *pte;

    // unsigned long phys_addr = 0;

    // // Find the VMA containing the address
    // vma = find_vma(mm, addr);
    // if (!vma) {
    //     seq_printf(m, "Address 0x%lx is not within any VMA\n", addr);
    //     return;
    // }

    // // Find the physical address translation
    // pte = get_locked_pte(mm, addr, &ptl);
    // if (!pte) {
    //     seq_printf(m, "Translation not found for address 0x%lx\n", addr);
    //     return;
    // }

    // phys_addr = pte_pfn(*pte) << PAGE_SHIFT;
    // pte_unmap_unlock(pte, ptl);
    // seq_printf(m, "Virtual address: 0x%lx, Physical address: 0x%lx\n", addr, phys_addr);
}

// Function to handle the "writeval" command
static void writeval_command(unsigned long addr, unsigned long val)
{
    // struct task_struct *task = current;
    unsigned long *ptr;

    // Verify the address is within the user space range
    if (addr < TASK_SIZE) {
        ptr = (unsigned long *)addr;
        if (access_ok((void __user *)addr, sizeof(unsigned long))) {
            // Write the new value
            if (put_user(val, ptr)) {
                printk("Failed to write value to address 0x%lx\n", addr);
                return;
            }

            printk("Value written successfully to address 0x%lx\n", addr);
            return;
        }
    }

    printk("Invalid address 0x%lx\n", addr);
}


// Write handler for /proc/mmaneg
static ssize_t mmaneg_proc_write(struct file *file, const char __user *buffer,
                                 size_t count, loff_t *pos)
{
    int command_length;

    if (count >= PROCFS_MAX_SIZE)
        count = PROCFS_MAX_SIZE - 1;

    // Copy the user buffer to the module's buffer
    if (copy_from_user(procfs_buffer, buffer, count))
        return -EFAULT;

    procfs_buffer[count] = '\0';
    procfs_buffer_size = count;

    // Handle the commands
    command_length = strlen(procfs_buffer);
    command = none;
    if (strncmp(procfs_buffer, "listvma", command_length) == 0) {
        // listvma_command(file->private_data);
        command = listvma;
    } else if (strncmp(procfs_buffer, "findpage", command_length) == 0) {
        sscanf(procfs_buffer, "findpage %lx", &address);
        // findpage_command(file->private_data, addr);
        command = findpage;
    } else if (strncmp(procfs_buffer, "writeval", command_length) == 0) {
        unsigned long addr, val;
        sscanf(procfs_buffer, "writeval %lx %lx", &addr, &val);
        writeval_command(addr, val);
    } else {
        printk("Unknown command: %s\n", procfs_buffer);
    }

    return count;
}


static void *my_seq_start(struct seq_file *s, loff_t *pos) 
{ 
    static int finished = 1;
    /* beginning a new sequence? */ 
    if (*pos == 0) { 
        /* yes => return a non null value to begin the sequence */ 
        return &finished; 
    } 
 
    /* no => it is the end of the sequence, return end to stop reading */ 
    *pos = 0; 
    return NULL; 
} 
 

static void *my_seq_next(struct seq_file *s, void *v, loff_t *pos) 
{  
    (*pos)++; 
    return NULL; 
} 
 
/* This function is called at the end of a sequence. */ 
static void my_seq_stop(struct seq_file *s, void *v) 
{ 
    /* nothing to do, we use a static value in start() */ 
} 
 
/* This function is called for each "step" of a sequence. */ 
static int my_seq_show(struct seq_file *s, void *v) 
{ 
    if (command == findpage) {
        findpage_command(s, address);
    } else if (command == listvma) {
        listvma_command(s);
    }
    return 0; 
} 
 
/* This structure gather "function" to manage the sequence */ 
static struct seq_operations my_seq_ops = { 
    .start = my_seq_start, 
    .next = my_seq_next, 
    .stop = my_seq_stop, 
    .show = my_seq_show, 
}; 
 
/* This function is called when the /proc file is open. */ 
static int my_open(struct inode *inode, struct file *file) 
{ 
    return seq_open(file, &my_seq_ops); 
}; 

// File operations for /proc/mmaneg
static const struct proc_ops mmaneg_proc_fops = {
    .proc_open = my_open,
    .proc_read = seq_read,
    .proc_write = mmaneg_proc_write,
    .proc_lseek = seq_lseek,
    .proc_release = seq_release,
};

// Module initialization function
static int __init mmaneg_init(void)
{
    command = none;
    mmaneg_entry = proc_create(PROCFS_NAME, 0644, NULL, &mmaneg_proc_fops);
    if (!mmaneg_entry)
        return -ENOMEM;

    printk(KERN_INFO "/proc/%s created\n", PROCFS_NAME);

    return 0;
}

// Module cleanup function
static void __exit mmaneg_exit(void)
{
    remove_proc_entry(PROCFS_NAME, NULL);
    printk(KERN_INFO "/proc/%s removed\n", PROCFS_NAME);
}

module_init(mmaneg_init);
module_exit(mmaneg_exit);
MODULE_LICENSE("GPL");
