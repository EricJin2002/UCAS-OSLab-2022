#include <os/lock.h>
#include <os/sched.h>
#include <os/list.h>
#include <atomic.h>
#include <assert.h> // for [p3]

barrier_t bars[BARRIER_NUM];

void init_barriers(void){
    for(int i=0;i<BARRIER_NUM;i++){
        spin_lock_init(&bars[i].lock);
        bars[i].handle_num=0;
        bars[i].block_queue.next=&bars[i].block_queue;
        bars[i].block_queue.prev=&bars[i].block_queue;
        bars[i].count=0;
    }
}

// we assume that goal can be changed by calls with the same key
// however, when the goal changes, 
// those already blocked won't be unblocked until another process calls wait
int do_barrier_init(int key, int goal){
    for(int i=0;i<BARRIER_NUM;i++){
        spin_lock_acquire(&bars[i].lock);
        if(bars[i].handle_num>0 && bars[i].key==key){
            bars[i].handle_num++;
            bars[i].goal=goal; // really?
            spin_lock_release(&bars[i].lock);
            return i;
        }
        spin_lock_release(&bars[i].lock);
    }
    for(int i=0;i<BARRIER_NUM;i++){
        spin_lock_acquire(&bars[i].lock);
        if(bars[i].handle_num==0){
            bars[i].handle_num++;
            assert(list_is_empty(&bars[i].block_queue));
            assert(bars[i].count==0);
            bars[i].key=key;
            bars[i].goal=goal;
            spin_lock_release(&bars[i].lock);
            return i;
        }
        spin_lock_release(&bars[i].lock);
    }
    return -1;
}

void do_barrier_wait(int bar_idx){
    spin_lock_acquire(&bars[bar_idx].lock);
    if(++bars[bar_idx].count>=bars[bar_idx].goal){
        list_node_t *node;
        while((node=list_pop(&bars[bar_idx].block_queue))){
            do_unblock(node);
        }
        bars[bar_idx].count=0;
    }else{
        do_block(&current_running->list, &bars[bar_idx].block_queue, &bars[bar_idx].lock);
    }
    spin_lock_release(&bars[bar_idx].lock);
}

void do_barrier_destroy(int bar_idx){
    spin_lock_acquire(&bars[bar_idx].lock);
    spin_lock_init(&bars[bar_idx].lock);
    bars[bar_idx].handle_num=0;
    list_node_t *node;
    while((node=list_pop(&bars[bar_idx].block_queue))){
        do_unblock(node);
    }
    bars[bar_idx].count=0;
    spin_lock_release(&bars[bar_idx].lock);
}