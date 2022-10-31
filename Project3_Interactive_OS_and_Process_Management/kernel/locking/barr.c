#include <os/lock.h>
#include <os/sched.h>
#include <os/list.h>
#include <atomic.h>
#include <assert.h> // for [p3]
#include <os/smp.h> // for [p3]

barrier_t barrs[BARRIER_NUM];

void init_barriers(void){
    for(int i=0;i<BARRIER_NUM;i++){
        spin_lock_init(&barrs[i].lock);
        barrs[i].handle_num=0;
        barrs[i].block_queue.next=&barrs[i].block_queue;
        barrs[i].block_queue.prev=&barrs[i].block_queue;
        barrs[i].count=0;
    }
}

// we assume that goal can be changed by calls with the same key
// however, when the goal changes, 
// those already blocked won't be unblocked until another process calls wait
int do_barrier_init(int key, int goal){
    for(int i=0;i<BARRIER_NUM;i++){
        spin_lock_acquire(&barrs[i].lock);
        if(barrs[i].handle_num>0 && barrs[i].key==key){
            barrs[i].handle_num++;
            barrs[i].goal=goal; // really?
            spin_lock_release(&barrs[i].lock);
            return i;
        }
        spin_lock_release(&barrs[i].lock);
    }
    for(int i=0;i<BARRIER_NUM;i++){
        spin_lock_acquire(&barrs[i].lock);
        if(barrs[i].handle_num==0){
            barrs[i].handle_num++;
            assert(list_is_empty(&barrs[i].block_queue));
            assert(barrs[i].count==0);
            barrs[i].key=key;
            barrs[i].goal=goal;
            spin_lock_release(&barrs[i].lock);
            return i;
        }
        spin_lock_release(&barrs[i].lock);
    }
    return -1;
}

void do_barrier_wait(int bar_idx){
    spin_lock_acquire(&barrs[bar_idx].lock);
    if(++barrs[bar_idx].count>=barrs[bar_idx].goal){
        list_node_t *node;
        while((node=list_pop(&barrs[bar_idx].block_queue))){
            do_unblock(node);
        }
        barrs[bar_idx].count=0;
    }else{
        do_block(&current_running_of[get_current_cpu_id()]->list, &barrs[bar_idx].block_queue, &barrs[bar_idx].lock);
    }
    spin_lock_release(&barrs[bar_idx].lock);
}

void do_barrier_destroy(int bar_idx){
    spin_lock_acquire(&barrs[bar_idx].lock);
    barrs[bar_idx].handle_num=0;
    list_node_t *node;
    while((node=list_pop(&barrs[bar_idx].block_queue))){
        do_unblock(node);
    }
    barrs[bar_idx].count=0;
    spin_lock_release(&barrs[bar_idx].lock);
}