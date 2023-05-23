#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/atomic.h>
#include <linux/types.h> 
#include <linux/spinlock.h>
#include <linux/mutex.h>

#include <linux/uaccess.h> 


#define MY_BLOCK_SZ 4096

struct my_deque_node
{
    
    u16 begin;
    u16 end;
    struct my_deque_node *next;
    u8 data[MY_BLOCK_SZ];
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

    dqe->head = kmalloc(sizeof(struct my_deque_node), GFP_KERNEL);
    init_my_node(dqe->head);
    dqe->tail = dqe->head;

}

s64 try_write_small(struct my_deque* dqe, const void __user * data, u64 len) {
    struct my_deque_node* tail;
    bool res;

    if (len > MY_BLOCK_SZ)
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

    if (MY_BLOCK_SZ <= tail->end + len) {
        if(copy_from_user(tail->data + tail->end, data, len)) {
            return -EFAULT;
        }
        tail->end += len;
        res = len;
    } else {
        res = 0;
    }
    
    spin_unlock(&(dqe->lock));
    return res;
}

void push_node_span(struct my_deque *dq, struct my_deque_node* head, struct my_deque_node* tail) {
    spin_lock(&(dq->lock));
        dq->tail->next = head;
        dq->tail = tail;
    spin_unlock(&(dq->lock));
}

s64 push_long(struct my_deque *dq, const void __user *data, u64 len) {
    struct my_deque_node* head = NULL, *tail=NULL, *new_node;
    u64 delta;

    while(len != 0) {
        new_node = kmalloc(sizeof(struct my_deque_node), GFP_KERNEL);
        init_my_node(new_node);
        
        delta = (len > MY_BLOCK_SZ ? MY_BLOCK_SZ : len);
        if(copy_from_user(new_node->data, data, delta)) {
            return -EFAULT;
        }
        data += delta;
        len -= delta;
        if(head == NULL) {
            head = new_node;
            tail = new_node;
        } else {
            tail->next = new_node;
            tail = new_node;
        }
    }
    
    push_node_span(dq, head, tail);
    return len;
}



s64 push_back(struct my_deque *dq, const void __user *data, u64 len)
{
    s64 res;
    if (! access_ok(data, len)){
        return(-EACCES);
    }

    if(len == 0) {
        return 0;
    }
    res = try_write_small(dq, data, len);
    if(res != 0) {
        return res;
    }
    return push_long(dq, data, len);   
}



s64 pop_front(struct my_deque *dq, void __user *data, u64 len) {
    
    u8 end_buf_loc[1024];
    u8* end_buf = end_buf_loc;
    struct my_deque_node* head = NULL, *finish = NULL, *old;
    u64 delta = 0, part = 0, full = 0;


    if (! access_ok(data, len)){
        return(-EACCES);
    }


    if (len > 1024) {
        end_buf = kmalloc(MY_BLOCK_SZ, GFP_KERNEL);
    } 

    
    spin_lock(&(dq->lock));
        head = dq->head;
        while(len + dq->head->begin > dq->head->end && dq->head != dq->tail) {
            len -= dq->head->end - dq->head->begin;
            dq->head = dq->head->next;
        }
        finish = dq->head;
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
            
            if(copy_to_user(data, dq->head->data + dq->head->begin, part)){
                if (len > 1024) {
                    kfree(end_buf);
                }
                return(-EFAULT);
            }
        }
        full += part;
        data += part;

        old = head;
        head = head->next;
        kfree(old);
    }
    if (delta != 0) {
        
        if(copy_to_user(data, end_buf, delta)) {
            if (len > 1024) {
                kfree(end_buf);
            }
            return(-EFAULT);
        }
        full += delta;
    }
    return full;
}
