#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <asm/atomic.h>
#include <asm/io.h>

#define PS2_KEYBOARD_IRQ 1

static atomic64_t = 0;

static u64 last_val;

static u8 round[60];
static u64 last_time=0;


void keyboard_tasklet_bh(unsigned long);

DECLARE_TASKLET(keyboard_tasklet, keyboard_tasklet_bh, 0);
 
void keyboard_tasklet_bh(unsigned long hits) {
    u64 val = atomic_read()
}



irqreturn_t ps2_keyboard_interrupt(int irq, void *dev_id) {
    unsigned char in = inb(0x60);
    if (!(in & (unsigned char)128)) {
        atomic_add(1, &scanned_keycode);
    }
    return IRQ_HANDLED;
}

static int __init ps2_keyboard_module_init(void) {
    int result;

    printk(KERN_INFO "PS/2 Keyboard Module: Initializing\n");

    // Зарегистрируем обработчик прерывания клавиатуры PS/2
    result = request_irq(PS2_KEYBOARD_IRQ, ps2_keyboard_interrupt, IRQF_SHARED, "ps2_keyboard", (void *)(ps2_keyboard_interrupt));
    if (result) {
        printk(KERN_ERR "PS/2 Keyboard Module: Failed to register interrupt handler\n");
        return result;
    }

    printk(KERN_INFO "PS/2 Keyboard Module: Initialized\n");

    return 0;
}

static void __exit ps2_keyboard_module_exit(void) {
    printk(KERN_INFO "PS/2 Keyboard Module: Exiting\n");

    // Освобождаем ресурсы и отключаем обработчик прерывания
    free_irq(PS2_KEYBOARD_IRQ, (void *)(ps2_keyboard_interrupt));

    printk(KERN_INFO "PS/2 Keyboard Module: Exited\n");
}

module_init(ps2_keyboard_module_init);
module_exit(ps2_keyboard_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("PS/2 Keyboard Module");

