#include <os/list.h>
#include <os/sched.h>
#include <type.h>

uint64_t time_elapsed = 0;
uint64_t time_base = 0;

uint64_t get_ticks()
{
    __asm__ __volatile__(
        "rdtime %0"
        : "=r"(time_elapsed));
    return time_elapsed;
}

uint64_t get_timer()
{
    return get_ticks() / time_base;
}

uint64_t get_time_base()
{
    return time_base;
}

void latency(uint64_t time)
{
    uint64_t begin_time = get_timer();

    while (get_timer() - begin_time < time);
    return;
}

void check_sleeping(void)
{
    // TODO: [p2-task3] Pick out tasks that should wake up from the sleep queue
    list_node_t *head = &sleep_queue;
    uint64_t curr_time = get_timer();
    while(head->next!=&sleep_queue){
        if(curr_time>=LIST2PCB(head->next)->wakeup_time){
            // printl("about to wake up\n\r");
            // list_node_t *node_to_wakeup = list_pop(head);
            do_unblock(list_pop(head));
            // list_push(&ready_queue, node_to_wakeup);
            // LIST2PCB(node_to_wakeup)->status = TASK_READY;
            // printl("wake up pid %d\n\r",LIST2PCB(node_to_wakeup)->pid);
        }else{
            head = head->next;
        }
    }
}