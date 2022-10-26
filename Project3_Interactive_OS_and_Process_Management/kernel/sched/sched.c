#include <os/list.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/time.h>
#include <os/mm.h>
#include <screen.h>
#include <printk.h>
#include <assert.h>
#include <csr.h>        // for [p2-task5]
#include <os/loader.h>  // for [p3-task1]
#include <os/string.h>  // for [p3-task1]
#include <os/task.h>    // for [p3-task1]

pcb_t pcb[NUM_MAX_TASK];
const ptr_t pid0_stack = INIT_KERNEL_STACK + PAGE_SIZE;
pcb_t pid0_pcb = {
    .pid = 0,
    .kernel_sp = (ptr_t)pid0_stack,
    .user_sp = (ptr_t)pid0_stack,
    .name = "kernel"
};

// for [p3]
// todo: add pcb lock logic
spin_lock_t pcb_lock;

LIST_HEAD(ready_queue);
LIST_HEAD(sleep_queue);

/* current running task PCB */
pcb_t * volatile current_running;

/* global process id */
pid_t process_id = 1;

// for [p3-task1]
const char *task_status_str[]={
    "BLOCKED",
    "RUNNING",
    "READY  ",
    "EXITED ",
    "UNUSED "
};

void do_scheduler(void)
{
    // TODO: [p2-task3] Check sleep queue to wake up PCBs

    check_sleeping();

    // TODO: [p2-task1] Modify the current_running pointer.
    //pcb_list_print(&ready_queue);

    pcb_t *prev_running = current_running;
    if(prev_running->status==TASK_RUNNING){
        // else, the task is blocked, don't push it to ready_queue
        // or is exited, don't push it to ready_queue (for [p2-task5])
        list_push(&ready_queue, &prev_running->list);
        prev_running->status = TASK_READY;
    }
    
    while(list_is_empty(&ready_queue)) {
        // even current_running doesn't want to work anymore
        // fine.. continuously check sleeping
        check_sleeping();
    }
    
    list_node_t *next_node;
    next_node = list_pop(&ready_queue);

    current_running = LIST2PCB(next_node);
    current_running->status = TASK_RUNNING;

    // printl("current ready_queue ");
    // pcb_list_print(&ready_queue);

    // printl("current sleep_queue ");
    // pcb_list_print(&sleep_queue);

    // TODO: [p2-task1] switch_to current_running
    // printl("switching from %d to %d\n\r", prev_running->pid, current_running->pid);
    switch_to(prev_running, current_running);
    screen_reflush();
}

void do_sleep(uint32_t sleep_time)
{
    // TODO: [p2-task3] sleep(seconds)
    // NOTE: you can assume: 1 second = 1 `timebase` ticks
    // 1. block the current_running
    // 2. set the wake up time for the blocked task
    // 3. reschedule because the current_running is blocked.

    // list_push(&sleep_queue, &current_running->list);
    // current_running->status = TASK_BLOCKED;
    // current_running->wakeup_time = get_timer() + sleep_time;
    // // printl("do_sleep pid %d time %d wakeup_time %d\n\r",current_running->pid,sleep_time,current_running->wakeup_time);
    // do_scheduler();

    // modified in [p3-task1]
    current_running->wakeup_time = get_timer() + sleep_time;
    do_block(&current_running->list, &sleep_queue, &pcb_lock);
}

void do_block(list_node_t *pcb_node, list_head *queue, spin_lock_t *lock)
{
    // for [p3]
    if(lock!=&pcb_lock){
        // todo: once add pcb lock logic, uncomment this
        // spin_lock_acquire(&pcb_lock);
        spin_lock_release(lock);
    }

    // TODO: [p2-task2] block the pcb task into the block queue
    list_push(queue, pcb_node);
    LIST2PCB(pcb_node)->status = TASK_BLOCKED;
    // printl("do_block pid %d\n\r",LIST2PCB(pcb_node)->pid);
    // pcb_list_print(&ready_queue);
    do_scheduler();
    // printl("!!!after do_scheduler\n\r");

    // for [p3]
    if(lock!=&pcb_lock){
        // todo: once add pcb lock logic, uncomment this
        // spin_lock_release(&pcb_lock);
        spin_lock_acquire(lock);
    }
}

void do_unblock(list_node_t *pcb_node)
{
    // TODO: [p2-task2] unblock the `pcb` from the block queue
    list_push(&ready_queue, pcb_node);
    LIST2PCB(pcb_node)->status = TASK_READY;
    // printl("do_unblock pid %d\n\r",LIST2PCB(pcb_node)->pid);
    // pcb_list_print(&ready_queue);
}

// for [p2-task5]
extern void ret_from_exception();
void thread_create(tid_t *tidptr, uint64_t entrypoint, long a0, long a1, long a2){
    // int i = process_id++;
    // pcb[i].pid = i;
    // pcb[i].status = TASK_READY;

    ptr_t kernel_stack = allocKernelPage(1) + PAGE_SIZE;
    ptr_t user_stack = allocUserPage(1) + PAGE_SIZE;

    tcb_t *tcb = (tcb_t *)kernel_stack;
    kernel_stack -= sizeof(tcb_t);
    tcb->pid = current_running->pid;
    list_node_t *father_node = &current_running->tcb_list;
    while(TCBLIST2TCB(father_node)->tid){
        father_node = father_node->next;
    }
    tcb->tid = TCBLIST2TCB(father_node->prev)->tid + 1; // todo: what if tid reaches its upper limit
    list_push(father_node, &tcb->tcb_list);
    *tidptr=tcb->tid;

    tcb->status = TASK_READY;

    regs_context_t *pt_regs =
        (regs_context_t *)(kernel_stack - sizeof(regs_context_t));

    pt_regs->sepc       = (reg_t) entrypoint;
    pt_regs->sstatus    = (reg_t) (SR_SPIE & ~SR_SPP);
    pt_regs->regs[2]    = (reg_t) user_stack;           //sp
    pt_regs->regs[4]    = (reg_t) tcb; //(pcb + i);     //tp
    pt_regs->regs[10]   = (reg_t) a0;
    pt_regs->regs[11]   = (reg_t) a1;
    pt_regs->regs[12]   = (reg_t) a2;
    // pt_regs->regs[13]   = (reg_t) a3;

    switchto_context_t *pt_switchto =
        (switchto_context_t *)((ptr_t)pt_regs - sizeof(switchto_context_t));
    
    pt_switchto->regs[0] = (reg_t) ret_from_exception;  //ra
    pt_switchto->regs[1] = (reg_t) pt_switchto;         //sp

    // printl("entrypoint %lx\n", entrypoint);

    tcb->kernel_sp = (reg_t) pt_switchto;
    tcb->user_sp = user_stack;

    list_push(&ready_queue, &tcb->list);
}

void thread_exit(void *retval){
    current_running->status = TASK_EXITED;
    current_running->retval = retval;
    do_scheduler();
}

// on success, return 0; on error, return -1
int thread_join(tid_t tid, void **retvalptr){
    list_node_t *joining_node = &current_running->tcb_list;
    do{
        joining_node = joining_node->next;
        if(joining_node==&current_running->tcb_list){
            return -1;
        }
    }while(TCBLIST2TCB(joining_node)->tid!=tid);

    tcb_t *joining_tcb = TCBLIST2TCB(joining_node);

    while(joining_tcb->status!=TASK_EXITED){
        do_scheduler();
    }

    *retvalptr = joining_tcb->retval;
    list_pop(joining_node->prev); // delete joining_node from tcb_list

    // todo: recycle stack after thread joins

    return 0;
}

// for main.c
regs_context_t *init_pcb_stack(
    ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point,
    pcb_t *pcb)
{
    // for [p3-task1]
    pcb->kernel_stack_base  = kernel_stack;
    pcb->user_stack_base    = user_stack;

     /* TODO: [p2-task3] initialization of registers on kernel stack
      * HINT: sp, ra, sepc, sstatus
      * NOTE: To run the task in user mode, you should set corresponding bits
      *     of sstatus(SPP, SPIE, etc.).
      */
    regs_context_t *pt_regs =
        (regs_context_t *)(kernel_stack - sizeof(regs_context_t));

    pt_regs->sepc       = (reg_t) entry_point;
    pt_regs->sstatus    = (reg_t) (SR_SPIE & ~SR_SPP);
    pt_regs->regs[2]    = (reg_t) user_stack;   //sp
    pt_regs->regs[4]    = (reg_t) pcb;          //tp

    /* TODO: [p2-task1] set sp to simulate just returning from switch_to
     * NOTE: you should prepare a stack, and push some values to
     * simulate a callee-saved context.
     */
    switchto_context_t *pt_switchto =
        (switchto_context_t *)((ptr_t)pt_regs - sizeof(switchto_context_t));
    
    pt_switchto->regs[0] = (reg_t) ret_from_exception;  //ra
    pt_switchto->regs[1] = (reg_t) pt_switchto;         //sp

    printl("entrypoint %lx\n", entry_point);

    pcb->kernel_sp = (reg_t) pt_switchto;
    pcb->user_sp = user_stack;

    return pt_regs;
}

// init pcb[i] by loading task named name
regs_context_t *init_pcb_via_name(int i, uint64_t entrypoint, char *taskname){
    assert(pcb[i].status==TASK_UNUSED);

    strncpy(pcb[i].name,taskname,32);

    pcb[i].pid = process_id++;
    pcb[i].status = TASK_READY;

    // for [p3-task1]
    pcb[i].wait_list.prev = &pcb[i].wait_list;
    pcb[i].wait_list.next = &pcb[i].wait_list;

    // for [p2-task5]
    pcb[i].tid = 0;
    pcb[i].tcb_list.prev = &pcb[i].tcb_list;
    pcb[i].tcb_list.next = &pcb[i].tcb_list;

    regs_context_t *pt_regs = init_pcb_stack(
        allocKernelPage(1) + PAGE_SIZE,
        allocUserPage(1) + PAGE_SIZE,
        entrypoint,
        pcb+i
    );
    list_push(&ready_queue, &pcb[i].list);

    return pt_regs;
}

#ifdef S_CORE
pid_t do_exec(int id, int argc, uint64_t arg0, uint64_t arg1, uint64_t arg2){
#else
pid_t do_exec(char *name, int argc, char *argv[]){
#endif
    for(int i=1;i<NUM_MAX_TASK;i++){
        if(pcb[i].status==TASK_UNUSED){

#ifdef S_CORE
            uint64_t entrypoint = load_task_img(id);
#else
            uint64_t entrypoint = load_task_img_via_name(name);
#endif

            if(!entrypoint){
                // is bat or no such task
                return 0;
            }

#ifdef S_CORE
            regs_context_t *pt_regs = init_pcb_via_name(i, entrypoint, tasks[id].name);
            pt_regs->regs[10] = argc;   //a0
            pt_regs->regs[11] = arg0;   //a1
            pt_regs->regs[12] = arg1;   //a2
            pt_regs->regs[13] = arg2;   //a3
#else
            regs_context_t *pt_regs = init_pcb_via_name(i, entrypoint, name);
            ptr_t user_sp_origin = pcb[i].user_sp;
            uint64_t *argv_base = (uint64_t *)(user_sp_origin-8*(argc+1));
            ptr_t user_sp_now = (ptr_t)argv_base;
            for(int i=0;i<argc;i++){
                user_sp_now -= strlen(argv[i])+1;
                strcpy((char *)user_sp_now, argv[i]);
                argv_base[i] = user_sp_now;
            }
            argv_base[argc] = (uint64_t)0;

            // alignment
            while((user_sp_origin-user_sp_now)%128){
                user_sp_now--;
            }

            // choose a place to save user_sp
            // according to entry.S
            // we now load user_sp from pcb
            pcb[i].user_sp = user_sp_now;
            pt_regs->regs[2] = (reg_t)user_sp_now; // so this line is not necessary

            pt_regs->regs[10] = (reg_t)argc;
            pt_regs->regs[11] = (reg_t)argv_base;

            // for(int i=0;i<argc;i++){
            //     printk("argv[%d]: %s\n", i, argv_base[i]);
            // }
#endif

            return pcb[i].pid;
        }
    }

    // no available pcb, return err
    return 0;
}

void do_exit(void){
    // current_running->status=TASK_EXITED;
    // list_node_t *node;
    // while(node=list_pop(&current_running->wait_list)){
    //     do_unblock(node);
    // }

    do_kill(current_running->pid);
}

int do_kill(pid_t pid){
    for(int i=1;i<process_id;i++){
        if(pcb[i].status!=TASK_UNUSED){
            if(pcb[i].pid==pid){
                pcb[i].status=TASK_EXITED;
                list_delete(&pcb[i].list);

                // wakeup all the node blocked on wait_list
                list_node_t *node;
                while((node=list_pop(&pcb[i].wait_list))){
                    do_unblock(node);
                }

                // todo: release occupied resources (such as locks)
                for(int j=0;j<LOCK_NUM;j++){
                    spin_lock_acquire(&mlocks[j].lock);
                    if(mlocks[j].owner==pid){
                        do_mutex_lock_release_compulsorily(j);
                    }
                    spin_lock_release(&mlocks[j].lock);
                }

                return 1;
            }
        }
    }
    return 0;
}

int do_waitpid(pid_t pid){
    for(int i=1;i<process_id;i++){
        if(pcb[i].status!=TASK_UNUSED){
            if(pcb[i].pid==pid){
                if(pcb[i].status!=TASK_EXITED){
                    do_block(&current_running->list, &pcb[i].wait_list, &pcb_lock);
                }
                return pid;
            }
        }
    }
    return 0;
}

pid_t do_getpid(){
    return current_running->pid;
}

void do_process_show(){
    printk("[Process Table]\n");
    printk("IDX PID STATUS TASK_NAME\n");
    for(int i=0;i<process_id;i++){
        if(pcb[i].status!=TASK_UNUSED){
            printk("[%d]  %d %s %s\n", 
                i, pcb[i].pid, task_status_str[pcb[i].status], pcb[i].name);
        }
    }
}

void do_task_show(){
    printk("[Task Table]\n");
    printk("IDX TYPE NAME\n");
    for(int i=0;i<task_num;i++){
        printk("[%d]  %s %s\n", 
            i, tasks[i].type==app?"APP":"BAT", tasks[i].name);
    }
    printk("Note: Not supported to run tasks with type BAT yet!\n");
}
