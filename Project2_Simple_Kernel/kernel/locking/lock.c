#include <os/lock.h>
#include <os/sched.h>
#include <os/list.h>
#include <atomic.h>

mutex_lock_t mlocks[LOCK_NUM];

void init_locks(void)
{
    /* TODO: [p2-task2] initialize mlocks */
    for(int i=0;i<LOCK_NUM;i++){
        mlocks[i].handle_num = 0;
        // not necessary
        // mlocks[i].lock.status = UNLOCKED;
    }
}

void spin_lock_init(spin_lock_t *lock)
{
    /* TODO: [p2-task2] initialize spin lock */
}

int spin_lock_try_acquire(spin_lock_t *lock)
{
    /* TODO: [p2-task2] try to acquire spin lock */
    return 0;
}

void spin_lock_acquire(spin_lock_t *lock)
{
    /* TODO: [p2-task2] acquire spin lock */
}

void spin_lock_release(spin_lock_t *lock)
{
    /* TODO: [p2-task2] release spin lock */
}

int do_mutex_lock_init(int key)
{
    /* TODO: [p2-task2] initialize mutex lock */
    for(int i=0;i<LOCK_NUM;i++){
        if(mlocks[i].handle_num>0 && mlocks[i].key==key){
            mlocks[i].handle_num++;
            printl("use mlock %d for key %d\n\r",i,key);
            return i;
        }
    }
    for(int i=1;i<LOCK_NUM;i++){
        if(mlocks[i].handle_num==0){
            mlocks[i].handle_num++;
            mlocks[i].lock.status=UNLOCKED;
            mlocks[i].block_queue.next=&mlocks[i].block_queue;
            mlocks[i].block_queue.prev=&mlocks[i].block_queue;
            mlocks[i].key=key;
            printl("allocate mlock %d for key %d\n\r",i,key);
            return i;
        }
    }
    return -1;
}

void do_mutex_lock_acquire(int mlock_idx)
{
    /* TODO: [p2-task2] acquire mutex lock */
    if(atomic_swap(LOCKED, (ptr_t)&mlocks[mlock_idx].lock.status)==LOCKED){
        do_block(&current_running->list, &mlocks[mlock_idx].block_queue);
        printl("!!!after do_block\n\r");
    }else{
        printl("mlock_idx %d locked\n\r", mlock_idx);
    }
}

void do_mutex_lock_release(int mlock_idx)
{
    /* TODO: [p2-task2] release mutex lock */
    // todo: still with bug if preemptive scheduling
    list_node_t *node_to_be_unblocked = list_pop(&mlocks[mlock_idx].block_queue);
    if(node_to_be_unblocked){
        do_unblock(node_to_be_unblocked);
    }else{
        mlocks[mlock_idx].lock.status = UNLOCKED;
        printl("mlock_idx %d unlocked\n\r", mlock_idx);
    }
}
