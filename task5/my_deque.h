#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/atomic.h>
#include <linux/types.h> 
#include <linux/spinlock.h>
#include <linux/mutex.h>


#define MY_BLOCK_SZ
struct my_deque_node
{
    u8 data[MY_BLOCK_SZ];
    u16 begin;
    u16 end;
    struct my_deque_node *next;
};

void init_my_node(struct my_deque_node* node) {
    node->begin = 0;
    node->end =0;
    node->next = NULL;
}

struct my_deque
{
    struct my_deque_node *head;
    struct my_deque_node *tail;
    spinlock_t lock;

};

void deque_init(struct my_deque* dqe) {
    spin_lock_init(&(dqe->lock));

    dqe->head = kmalloc(sizeof(my_deque_node), GFP_KERNEL);
    init_my_node(&(dqe->head));
    dqe->tail = dqe->head;

}


bool try_write_small(struct my_deque* dqe, const void __user * data, u64 len) {
    struct my_deque_node* tail;
    bool res;

    if (size > BLOCK_SIZE)
    {
        return 0;
    }
    
    spin_lock(&(dqe->lock));
    tail = dqe->tail;


    if (tail == NULL)
    {
        spin_unlock(&(dqe->lock));
        printk("impossible state");
        return 0;
    }

    if (4096 <= tail->end + len) {
        copy_from_user(tail->data + tail->end, data, len);
        tail->end += len;
        res = 1;
    } else {
        res = 0;
        if(len <  MY_BLOCK_SZ/4) {
            atomic_write(&(dqe->need_alloc), 1);
        }
    }
    
    spin_unlock(&(dqe->lock));
    return res;
}

void push_node_span(struct deque *dq, struct my_deque_node* head, struct my_deque_node* tail) {
    spin_lock(&(dq->lock));
        dq->tail->next = head;
        dq->tail = tail;
    spin_unlock(&(dq->lock));
}

void push_long(struct deque *dq, const void __user *data, u64 len) {
    struct my_deque_node* head = NULL, tail=NULL, new_node;
    u64 delta;

    while(len != 0) {
        new_node = kmalloc(sizeof(my_deque_node), GFP_KERNEL);
        init_my_node(new_node);
        
        delta = (len > MY_BLOCK_SZ ? MY_BLOCK_SZ : len);
        copy_from_user(new_node->data, data, delta);
        data += delta
        len -= delta;
        if(head == NULL) {
            head = new_node;
            tail = new_node;
        } else {
            tail->next = new_node;
            tail = newstruct my_deque_node* _node;
        }
    }
    
    push_node_span(dq, head, tail);
}



u64 push_back(struct deque *dq, void *data, u64 len)
{
    if(len == 0) {
        return 0;
    }
    if(try_write_small(dq, data, len)) {
        return len;
    }
    push_long(dq, data, len);
    return len;    
}



u64 pop_front(struct deque *dq, const void __user *data, u64 len) {
    u8 end_buf[MY_BLOCK_SZ];
    struct my_deque_node* head = NULL, finish = NULL, old;
    u64 delta = 0, part = 0, full = 0;

    spin_lock(&(dq->lock));
        head = dq->head;
        while(len + dq->head->begin > dq->head->end && dq->head != dq->tail) {
            len -= dq->head->end - dq->head->begin;
            dq->head = dq->head->next;
        }
        finish = qd->head;
        delta = dq->head->end - dq->head->begin;
        delta = (len > delta ? delta : len);
        if (delta != 0) {
            memcpy(end_buf, dq->head->data + dq->head->begin, delta);
            dq->head->begin += delta;
        }
    spin_unlock(&(dq->lock));

    for (; head != finish; )
    {
        part = head->end - head->begin;
        if (part != 0){
            copy_to_user(data, dq->head->data + dq->head->begin, part);
        }
        full += part;
        data += part;

        old = head;
        head = head->next;
        kfree(old);
    }
    if (delta != 0) {
        copy_to_user(data, end_buf, delta);
        full += delta;
    }
    return full;
}
