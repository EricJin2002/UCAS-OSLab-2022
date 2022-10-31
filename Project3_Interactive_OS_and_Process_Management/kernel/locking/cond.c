#include <os/lock.h>
#include <os/sched.h>
#include <os/list.h>
#include <atomic.h>
#include <assert.h> // for [p3]
#include <os/smp.h> // for [p3]

condition_t conds[CONDITION_NUM];

void init_conditions(void){
    for(int i=0;i<CONDITION_NUM;i++){
        spin_lock_init(&conds[i].lock);
        conds[i].handle_num=0;
        conds[i].block_queue.next=&conds[i].block_queue;
        conds[i].block_queue.prev=&conds[i].block_queue;
    }
}

int do_condition_init(int key){
    for(int i=0;i<CONDITION_NUM;i++){
        spin_lock_acquire(&conds[i].lock);
        if(conds[i].handle_num>0 && conds[i].key==key){
            conds[i].handle_num++;
            spin_lock_release(&conds[i].lock);
            return i;
        }
        spin_lock_release(&conds[i].lock);
    }
    for(int i=0;i<CONDITION_NUM;i++){
        spin_lock_acquire(&conds[i].lock);
        if(conds[i].handle_num==0){
            conds[i].handle_num++;
            assert(list_is_empty(&conds[i].block_queue));
            conds[i].key=key;
            spin_lock_release(&conds[i].lock);
            return i;
        }
        spin_lock_release(&conds[i].lock);
    }
    return -1;
}

void do_condition_wait(int cond_idx, int mutex_idx){
    spin_lock_acquire(&conds[cond_idx].lock);
    do_mutex_lock_release(mutex_idx);
    do_block(&current_running_of[get_current_cpu_id()]->list, &conds[cond_idx].block_queue, &conds[cond_idx].lock);
    spin_lock_release(&conds[cond_idx].lock);
    do_mutex_lock_acquire(mutex_idx);
}

void do_condition_signal(int cond_idx){
    spin_lock_acquire(&conds[cond_idx].lock);
    list_node_t *node;
    if((node=list_pop(&conds[cond_idx].block_queue))){
        do_unblock(node);
    }
    spin_lock_release(&conds[cond_idx].lock);
}

void do_condition_broadcast(int cond_idx){
    spin_lock_acquire(&conds[cond_idx].lock);
    list_node_t *node;
    while((node=list_pop(&conds[cond_idx].block_queue))){
        do_unblock(node);
    }
    spin_lock_release(&conds[cond_idx].lock);
}

void do_condition_destroy(int cond_idx){
    spin_lock_acquire(&conds[cond_idx].lock);
    conds[cond_idx].handle_num=0;
    list_node_t *node;
    while((node=list_pop(&conds[cond_idx].block_queue))){
        do_unblock(node);
    }
    spin_lock_release(&conds[cond_idx].lock);
}