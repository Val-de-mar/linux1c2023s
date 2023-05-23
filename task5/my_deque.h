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
    node->end = 0;
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
    s64 res;
    printk("trying to write small %llu", len);

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

    // printk("Bloc_sz %d, end %d, len %d", (int)(MY_BLOCK_SZ), (int)(tail->end), (int)(len));
    if (MY_BLOCK_SZ >= tail->end + len) {
        if(copy_from_user(tail->data + tail->end, data, len) != 0) {
            spin_unlock(&(dqe->lock));
            printk("oh ho");
            return -EFAULT;
        }
        tail->end += len;
        res = len;
        // printk("Bloc_sz %d, end %d, len %d");
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
    s64 ans = len;
    int rr;
    // printk("inside pl, ");

    while(len != 0) {
        new_node = kmalloc(sizeof(struct my_deque_node), GFP_KERNEL);
        init_my_node(new_node);
        
        delta = (len > MY_BLOCK_SZ ? MY_BLOCK_SZ : len);
        rr = copy_from_user(new_node->data, data, delta);
        new_node->end = delta;
        printk("rr =  %d, delta = %d", (int)rr, (int)delta);

        if(rr != 0) {
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

    return ans;
}



s64 push_back(struct my_deque *dq, const void __user *data, u64 len)
{
    s64 res, ans;
    // printk("inside pb");
    if (! access_ok(data, len)){
        return(-EACCES);
    }

    if(len == 0) {
        return 0;
    }

    // printk("inside tws");
    res = try_write_small(dq, data, len);

    printk("res = %d", (int)res);
    if(res != 0) {
        return res;
    }
    ans = push_long(dq, data, len);

    printk("pushed long = %d", (int)ans);
    return ans;
}



s64 pop_front(struct my_deque *dq, void __user *data, u64 len) {
    
    u8 end_buf_loc[1024];
    u8* end_buf = end_buf_loc;
    bool is_allocated = (len > 1024);

    struct my_deque_node* head = NULL, *finish = NULL, *old;
    u64 part = 0;
    u64 unacquired_tail = 0;
    s64 full = 0;
    printk("full1 %lld", full);

    if (! access_ok(data, len)){
        printk("exit 1");
        return(-EACCES);
    }


    if (is_allocated) {
        end_buf = kmalloc(MY_BLOCK_SZ, GFP_KERNEL);
    } 
    printk("full2 %lld", full);
    
    spin_lock(&(dq->lock));
        head = dq->head;
        while(len + dq->head->begin > dq->head->end && dq->head != dq->tail) {
            len -= dq->head->end - dq->head->begin;
            dq->head = dq->head->next;
        }
        finish = dq->head;
        unacquired_tail = dq->head->end - dq->head->begin;
        unacquired_tail = (len > unacquired_tail ? unacquired_tail : len);
        if (unacquired_tail != 0) {
            memcpy(end_buf, dq->head->data + dq->head->begin, unacquired_tail);
            dq->head->begin += unacquired_tail;
        }
    spin_unlock(&(dq->lock));
    printk("full3 %lld", full);
    for (; head != finish; )
    {
        printk("inside!!!");
        part = head->end - head->begin;
        if (part != 0){
            if(copy_to_user(data, head->data + head->begin, part) != 0){
                if (is_allocated) {
                    kfree(end_buf);
                }
                printk("exit 2");
                return(-EFAULT);
            }
        }
        full += part;
        data += part;

        old = head;
        head = head->next;
        // kfree(old);
    }
    printk("full %lld", full);
    if (unacquired_tail != 0) {
        printk("Unracq");
        if(copy_to_user(data, end_buf, unacquired_tail) != 0) {
            if (is_allocated) {
                kfree(end_buf);
            }
            printk("exit 3");
            return(-EFAULT);
        }
        full += unacquired_tail;
    }
    printk("exit 4 %lld", full);
    return full;
}
