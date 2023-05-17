#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/uaccess.h>

#define PROCFS_MAX_SIZE 1024
#define PROCFS_NAME "mmaneg"

static struct proc_dir_entry *mmaneg_entry;
static char procfs_buffer[PROCFS_MAX_SIZE];
static unsigned long procfs_buffer_size = 0;

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
    struct task_struct *task = current;
    struct mm_struct *mm = task->mm;
    struct vm_area_struct *vma;
    pte_t *pte;
    unsigned long phys_addr = 0;

    // Find the VMA containing the address
    vma = find_vma(mm, addr);
    if (!vma) {
        seq_printf(m, "Address 0x%lx is not within any VMA\n", addr);
        return;
    }

    // Find the physical address translation
    pte = virt_to_pte(vma->vm_mm, addr);
    if (!pte) {
        seq_printf(m, "Translation not found for address 0x%lx\n", addr);
        return;
    }

    phys_addr = pte_pfn(*pte) << PAGE_SHIFT;
    seq_printf(m, "Virtual address: 0x%lx, Physical address: 0x%lx\n", addr, phys_addr);
}

// Function to handle the "writeval" command
static void writeval_command(struct seq_file *m, unsigned long addr, unsigned long val)
{
    struct task_struct *task = current;
    struct mm_struct *mm = task->mm;
    unsigned long *ptr;

    // Verify the address is within the user space range
    if (addr < TASK_SIZE) {
        ptr = (unsigned long *)addr;
        if (access_ok((void __user *)addr, sizeof(unsigned long))) {
            // Write the new value
            if (put_user(val, ptr)) {
                seq_printf(m, "Failed to write value to address 0x%lx\n", addr);
                return;
            }

            seq_printf(m, "Value written successfully to address 0x%lx\n", addr);
            return;
        }
    }

    seq_printf(m, "Invalid address 0x%lx\n", addr);
}

// Read handler for /proc/mmaneg
static int mmaneg_proc_show(struct seq_file *m, void *v)
{
    seq_printf(m, "%s", procfs_buffer);
    return 0;
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
    if (strncmp(procfs_buffer, "listvma", command_length) == 0) {
        listvma_command(file->private_data);
    } else if (strncmp(procfs_buffer, "findpage", command_length) == 0) {
        unsigned long addr;
        sscanf(procfs_buffer, "findpage %lx", &addr);
        findpage_command(file->private_data, addr);
    } else if (strncmp(procfs_buffer, "writeval", command_length) == 0) {
        unsigned long addr, val;
        sscanf(procfs_buffer, "writeval %lx %lx", &addr, &val);
        writeval_command(file->private_data, addr, val);
    } else {
        seq_printf(file->private_data, "Unknown command: %s\n", procfs_buffer);
    }

    *pos = 0; // Reset file position to start

    return count;
}

// File operations for /proc/mmaneg
static const struct file_operations mmaneg_proc_fops = {
    .owner = THIS_MODULE,
    .open = seq_open,
    .read = seq_read,
    .write = mmaneg_proc_write,
    .llseek = seq_lseek,
    .release = seq_release,
};

// Module initialization function
static int __init mmaneg_init(void)
{
    mmaneg_entry = proc_create(PROCFS_NAME, 0, NULL, &mmaneg_proc_fops);
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
