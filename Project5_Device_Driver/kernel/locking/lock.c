#include <os/lock.h>
#include <os/sched.h>
#include <os/list.h>
#include <atomic.h>
#include <assert.h> // for [p3]
#include <os/smp.h> // for [p3]

mutex_lock_t mlocks[LOCK_NUM];

// must done before wakeup other cores
void init_locks(void)
{
    /* TODO: [p2-task2] initialize mlocks */
    for(int i=0;i<LOCK_NUM;i++){
        spin_lock_init(&mlocks[i].lock);
        mlocks[i].handle_num=0;
        mlocks[i].status=UNLOCKED;
        mlocks[i].block_queue.next=&mlocks[i].block_queue;
        mlocks[i].block_queue.prev=&mlocks[i].block_queue;
        mlocks[i].owner=(uint64_t)0;
    }
    spin_lock_init(&pcb_lock);
}

void spin_lock_init(spin_lock_t *lock)
{
    /* TODO: [p2-task2] initialize spin lock */
    atomic_swap(UNLOCKED, (ptr_t)&lock->status);
    lock->owner=-1;
}

// on success, return 1; else, return 0
int spin_lock_try_acquire(spin_lock_t *lock)
{
    /* TODO: [p2-task2] try to acquire spin lock */
    if(atomic_swap(LOCKED, (ptr_t)&lock->status)){
        return 0;
    }else{
        lock->owner=get_current_cpu_id();
        return 1;
    }
}

void spin_lock_acquire(spin_lock_t *lock)
{
    /* TODO: [p2-task2] acquire spin lock */
    while(atomic_swap(LOCKED, (ptr_t)&lock->status));
    lock->owner=get_current_cpu_id();
}

void spin_lock_release(spin_lock_t *lock)
{
    /* TODO: [p2-task2] release spin lock */
    lock->owner=-1;
    assert(atomic_swap(UNLOCKED, (ptr_t)&lock->status));
}

int do_mutex_lock_init(int key)
{
    /* TODO: [p2-task2] initialize mutex lock */
    for(int i=0;i<LOCK_NUM;i++){
        spin_lock_acquire(&mlocks[i].lock);
        if(mlocks[i].handle_num>0 && mlocks[i].key==key){
            mlocks[i].handle_num++;
            // printl("use mlock %d for key %d\n\r",i,key);
            spin_lock_release(&mlocks[i].lock);
            return i;
        }
        spin_lock_release(&mlocks[i].lock);
    }
    for(int i=0;i<LOCK_NUM;i++){
        spin_lock_acquire(&mlocks[i].lock);
        if(mlocks[i].handle_num==0){
            mlocks[i].handle_num++;
            assert(mlocks[i].status==UNLOCKED);
            assert(mlocks[i].owner==0);
            assert(list_is_empty(&mlocks[i].block_queue));
            mlocks[i].key=key;
            // printl("allocate mlock %d for key %d\n\r",i,key);
            spin_lock_release(&mlocks[i].lock);
            return i;
        }
        spin_lock_release(&mlocks[i].lock);
    }
    return -1;
}

void do_mutex_lock_acquire(int mlock_idx)
{
    /* TODO: [p2-task2] acquire mutex lock */
    spin_lock_acquire(&mlocks[mlock_idx].lock);
    // not necessary to use atomic here
    // due to the protection of spin lock
    if(atomic_swap(LOCKED, (ptr_t)&mlocks[mlock_idx].status)==LOCKED){
        do_block(&current_running_of[get_current_cpu_id()]->list, &mlocks[mlock_idx].block_queue, &mlocks[mlock_idx].lock);
        // printl("!!!after do_block\n\r");
    }else{
        // printl("mlock_idx %d owner changed from %d to %d\n",mlock_idx,0,current_running_of[get_current_cpu_id()]->pid);
        mlocks[mlock_idx].owner=current_running_of[get_current_cpu_id()]->pid;
    }

    spin_lock_release(&mlocks[mlock_idx].lock);
}

// not about to check if is owener
// calling requires mlocks[mlock_idx].lock holding
void do_mutex_lock_release_compulsorily(int mlock_idx){
    list_node_t *node_to_be_unblocked = list_pop(&mlocks[mlock_idx].block_queue);
    if(node_to_be_unblocked){
        // printl("mlock_idx %d owner changed from %d to %d\n",mlock_idx,mlocks[mlock_idx].owner,LIST2PCB(node_to_be_unblocked)->pid);
        mlocks[mlock_idx].owner=LIST2PCB(node_to_be_unblocked)->pid;
        do_unblock(node_to_be_unblocked);
    }else{
        // printl("mlock_idx %d owner changed from %d to %d\n",mlock_idx,mlocks[mlock_idx].owner,0);
        mlocks[mlock_idx].owner=0;
        mlocks[mlock_idx].status = UNLOCKED;
    }
}

void do_mutex_lock_release(int mlock_idx)
{
    /* TODO: [p2-task2] release mutex lock */
    spin_lock_acquire(&mlocks[mlock_idx].lock);
    
    // check if holding
    assert(mlocks[mlock_idx].owner==current_running_of[get_current_cpu_id()]->pid);

    do_mutex_lock_release_compulsorily(mlock_idx);
    spin_lock_release(&mlocks[mlock_idx].lock);
}
