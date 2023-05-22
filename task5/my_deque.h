#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/atomic.h> 

struct deque_node
{
    u8 data[4096];
    atomic_t begin;
    atomic_t end;
    struct deque_node *next;
};

struct deque
{

    struct deque_node *head;
    struct deque_node *tail;
};

void __push_back_simple(struct deque_node *dq, void *data, u64 len)
{
    if(len == 0) {
        return;
    }
    memcpy(dq->data + atomic_read(&(dq->end)), data, len);
    atomic_add(&(dq->end), len);
}

u64 push_back(struct deque *dq, void *data, u64 len)
{
    void *end += len;
    u64 delta = 4096 - dq->tail->end;
    delta = delta<len?delta:len;
    __push_back_simple(dq->tail, data, delta);
    data += delta;
    len -= delta;
    while (len != 0)
    {
        struct deque_node* created = kmalloc(sizeof(deque_node), GFP_KERNEL);
        dq->tail->data
    }
    
}
