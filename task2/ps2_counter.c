#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/ktime.h>
#include <linux/interrupt.h>
#include <linux/string.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <asm/atomic.h>
#include <asm/io.h>

#define PS2_KEYBOARD_IRQ 1

static atomic64_t ans = 0;



static u8 round[60];

static u64 last_time=0;


void keyboard_tasklet_bh(unsigned long);

DECLARE_TASKLET(keyboard_tasklet, keyboard_tasklet_bh, 0);

i64 delta_on_segment(u8* from, u8* to) {
    i64 delta = 0;
    for(u8* ptr = from; ptr < to; ++ptr) {
        delta += *ptr;
        *ptr = 0;
    }
    return delta;        
}

void update_round() {
    u64 point = ktime_get_seconds();
    if (point == last_time)
    {
        atomic_add(&ans, 1);
    }
    
    if(point - last_time >= 60){
        i64 delta = -delta_on_segment(round, round + 60);
        ++delta;
        atomic_add(&ans, delta);
        return;
    }
    u64 from = last_time % 60;
    u64 to = point % 60;
    if (to > from) {
        i64 delta = -delta_on_segment(round + from, round + to);
        ++delta;
        atomic_add(&ans, delta);
        return;
    }
    i64 delta = -delta_on_segment(round + from, round + 60);
    delta -= delta_on_segment(round, round + to);
    ++delta;
    atomic_add(&ans, delta);
}
 
void keyboard_tasklet_bh(unsigned long hits) {
    unsigned char in = inb(0x60);
    if (!(in & (unsigned char)128)) {
        // atomic_add(1, &scanned_keycode);
        update_round();
    }
}

// Define your function to be executed periodically
void logger(unsigned long data)
{
    printk(KERN_INFO "typed in the last minute: %lld\n", atomic_read(&ans));

    // Reschedule the timer for the next minute
    mod_timer(&my_timer, jiffies + msecs_to_jiffies(60 * 1000));
}

// Declare a timer structure
static struct timer_list my_timer;


irqreturn_t ps2_keyboard_interrupt(int irq, void *dev_id) {
    tasklet_schedule(&keyboard_tasklet);
    return IRQ_HANDLED;
}

static int __init ps2_keyboard_module_init(void) {
    int result;

    printk(KERN_INFO "PS/2 Keyboard Module: Initializing\n");
    memset(round, sizeof(round), 0);

    setup_timer(&my_timer, logger, 0);

    // Set the initial expiration time (after 1 minute)
    mod_timer(&my_timer, jiffies + msecs_to_jiffies(60 * 1000));


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
MODULE_AUTHOR("Vladimir Melnikov");
MODULE_DESCRIPTION("PS/2 Keyboard Module");

