#include <os/list.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/time.h>
#include <os/mm.h>
#include <screen.h>
#include <printk.h>
#include <assert.h>

pcb_t pcb[NUM_MAX_TASK];
const ptr_t pid0_stack = INIT_KERNEL_STACK + PAGE_SIZE;
pcb_t pid0_pcb = {
    .pid = 0,
    .kernel_sp = (ptr_t)pid0_stack,
    .user_sp = (ptr_t)pid0_stack
};

LIST_HEAD(ready_queue);
LIST_HEAD(sleep_queue);

/* current running task PCB */
pcb_t * volatile current_running;

/* global process id */
pid_t process_id = 1;

void do_scheduler(void)
{
    // TODO: [p2-task3] Check sleep queue to wake up PCBs

    check_sleeping();

    // TODO: [p2-task1] Modify the current_running pointer.
    //pcb_list_print(&ready_queue);

    pcb_t *prev_running = current_running;
    if(prev_running->status==TASK_RUNNING){
        // else, the task is blocked, don't push it to ready_queue
        list_push(&ready_queue, &prev_running->list);
        prev_running->status = TASK_READY;
    }
    
    list_node_t *next_node;
    if(list_is_empty(&ready_queue)) {
        do_scheduler(); // nothing to do 
        return;
    }else{
        next_node = list_pop(&ready_queue);
    }

    current_running = LIST2PCB(next_node);
    current_running->status = TASK_RUNNING;

    printl("current ready_queue ");
    pcb_list_print(&ready_queue);

    printl("current sleep_queue ");
    pcb_list_print(&sleep_queue);

    // TODO: [p2-task1] switch_to current_running
    printl("switching from %d to %d\n\r", prev_running->pid, current_running->pid);
    switch_to(prev_running, current_running);
    printl("\n\r");
}

void do_sleep(uint32_t sleep_time)
{
    // TODO: [p2-task3] sleep(seconds)
    // NOTE: you can assume: 1 second = 1 `timebase` ticks
    // 1. block the current_running
    // 2. set the wake up time for the blocked task
    // 3. reschedule because the current_running is blocked.
    list_push(&sleep_queue, &current_running->list);
    current_running->status = TASK_BLOCKED;
    current_running->wakeup_time = get_timer() + sleep_time;
    printl("do_sleep pid %d time %d wakeup_time %d\n\r",current_running->pid,sleep_time,current_running->wakeup_time);
    do_scheduler();
}

void do_block(list_node_t *pcb_node, list_head *queue)
{
    // TODO: [p2-task2] block the pcb task into the block queue
    list_push(queue, pcb_node);
    LIST2PCB(pcb_node)->status = TASK_BLOCKED;
    printl("do_block pid %d\n\r",LIST2PCB(pcb_node)->pid);
    //pcb_list_print(&ready_queue);
    do_scheduler();
    printl("!!!after do_scheduler\n\r");
}

void do_unblock(list_node_t *pcb_node)
{
    // TODO: [p2-task2] unblock the `pcb` from the block queue
    list_push(&ready_queue, pcb_node);
    LIST2PCB(pcb_node)->status = TASK_READY;
    printl("do_unblock pid %d\n\r",LIST2PCB(pcb_node)->pid);
    //pcb_list_print(&ready_queue);
}
