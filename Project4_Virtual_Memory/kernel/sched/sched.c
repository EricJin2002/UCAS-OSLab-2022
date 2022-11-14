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
#include <os/smp.h>     // for [p3-task3]
#include <os/string.h>     // for [p3-task4]

pcb_t pcb[NUM_MAX_TASK];
const ptr_t pid0_stack = INIT_KERNEL_STACK + PAGE_SIZE;
pcb_t pid0_pcb;

// for [p3]
// todo: add pcb lock logic
spin_lock_t pcb_lock;

LIST_HEAD(ready_queue);
LIST_HEAD(sleep_queue);

/* current running task PCB */
// pcb_t * volatile current_running;
// register pcb_t * volatile current_running asm("tp"); // not working

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

static int filter_mask(list_node_t *node){
    return LIST2PCB(node)->mask&(1<<get_current_cpu_id());
}

void do_scheduler(void)
{
    // TODO: [p2-task3] Check sleep queue to wake up PCBs

    check_sleeping();

    // TODO: [p2-task1] Modify the current_running pointer.
    // printl("[core %d] before scheduler: ",get_current_cpu_id());pcb_list_print(&ready_queue);
    // printl("[core %d] enter with pcb [status %s]\n",get_current_cpu_id(),task_status_str[current_running_of[get_current_cpu_id()]->status]);

    pcb_t *prev_running = current_running_of[get_current_cpu_id()];
    if(!prev_running->pid){
        // if kernel, don't push it to ready_queue
        prev_running->status=TASK_BLOCKED;
    }
    if(prev_running->status==TASK_RUNNING){
        // else, the task is blocked, don't push it to ready_queue
        // or is exited, don't push it to ready_queue (for [p2-task5])
        list_push(&ready_queue, &prev_running->list);
        prev_running->status = TASK_READY;
    }
    prev_running->running_core = -1;
    
    list_node_t *next_node;

    // note: the below logic may encounter error, (thus we have abandoned this)
    // when the other core switch to this process 
    // (which is already blocked, looping to find an available task from ready_queue),
    // because there's NO switchto_context on the kernel stack!!

    // while(!(next_node=list_find_and_pop(&ready_queue,(void *)filter_mask))){
    //     unlock_kernel();
    //     lock_kernel();
    //     check_sleeping();
    // }

    if(!(next_node=list_find_and_pop(&ready_queue,(void *)filter_mask))){
        next_node=&kernel_pcb_of[get_current_cpu_id()]->list;
    }

//    while(list_is_empty(&ready_queue)) {
//        // even current_running doesn't want to work anymore
//        // fine.. continuously check sleeping
//
//        check_sleeping();
//        unlock_kernel();
//        
//        lock_kernel();
//    }
//    
//    list_node_t *next_node;
//    next_node = list_pop(&ready_queue);

    current_running_of[get_current_cpu_id()] = LIST2PCB(next_node);
    current_running_of[get_current_cpu_id()]->status = TASK_RUNNING;
    current_running_of[get_current_cpu_id()]->running_core = get_current_cpu_id();

    // printl("current ready_queue ");
    // pcb_list_print(&ready_queue);

    // printl("current sleep_queue ");
    // pcb_list_print(&sleep_queue);

    // for [p3-task3]
    // if(/*how to judge this task is first scheded?*/){
    //     // first sched, realse kernel lock
    //     unlock_kernel();
    // }

    printl("[core %d] switching from [pid %d name %s] to [pid %d name %s]\n",
        get_current_cpu_id(),
        prev_running->pid,
        prev_running->name,
        current_running_of[get_current_cpu_id()]->pid,
        current_running_of[get_current_cpu_id()]->name
    );
    printl("[core %d] after  scheduler: ",get_current_cpu_id());pcb_list_print(&ready_queue);
    printl("\n");

    // unlock_kernel();

    // printl("[before set_satp]\n");
    // print_va_at_pgdir(0x1001e, current_running_of[get_current_cpu_id()]->pgdir);
    // printl("\n");

    // for [p4-task1]
    set_satp(
        SATP_MODE_SV39, 
        current_running_of[get_current_cpu_id()]->pid,
        kva2pa(current_running_of[get_current_cpu_id()]->pgdir) >> NORMAL_PAGE_SHIFT
    );
    local_flush_tlb_all();

    // TODO: [p2-task1] switch_to current_running
    // printl("switching from %d to %d\n\r", prev_running->pid, current_running->pid);
    switch_to(prev_running, current_running_of[get_current_cpu_id()]);

    // lock_kernel();

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
    current_running_of[get_current_cpu_id()]->wakeup_time = get_timer() + sleep_time;
    do_block(&current_running_of[get_current_cpu_id()]->list, &sleep_queue, &pcb_lock);
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
    printl("do_unblock pid %d\n\r",LIST2PCB(pcb_node)->pid);
    pcb_list_print(&ready_queue);
}

extern void ret_from_exception();

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
    pt_regs->sstatus    = (reg_t) ((SR_SPIE & ~SR_SPP) | SR_SUM);
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

// init pcb[i] by loading app with id `taskid`
regs_context_t *init_pcb_via_id(int i, int taskid){
    assert(pcb[i].status==TASK_UNUSED);
    assert(taskid>=0 && taskid<task_num);
    assert(tasks[taskid].type==app);

    // for [p4-task3]
    assert(list_is_empty(&pcb[i].pf_list));
    assert(list_is_empty(&pcb[i].swp_list));

    // for [p3-task1]
    assert(list_is_empty(&pcb[i].wait_list));

    strncpy(pcb[i].name,tasks[taskid].name,32);

    pcb[i].pid = process_id++;
    pcb[i].status = TASK_READY;

    // for [p3-task3] & [p3-task4]
    pcb[i].running_core = -1;
    // inherit mask from its father
    pcb[i].mask = current_running_of[get_current_cpu_id()]->mask;

    // for [p2-task5]
    pcb[i].tid = 0;
    pcb[i].tcb_list.prev = &pcb[i].tcb_list;
    pcb[i].tcb_list.next = &pcb[i].tcb_list;

    // for [p4-task1]
    // alloc a new pgdir
    if(!pcb[i].pgdir){
        // haven't alloc before
        // no page to reuse
        pcb[i].pgdir = allocPage(1);
    }
    // init pgdir
    clear_pgdir(pcb[i].pgdir);
    share_pgtable(pcb[i].pgdir, current_running_of[get_current_cpu_id()]->pgdir);
    
    printl("[after share_pgtable]\n");
    print_va_at_pgdir(pa2kva(PGDIR_PA), pcb[i].pgdir);
    printl("\n");

    // note: the init of pcb[i].pid & pcb[i].pgdir must be put above

    // alloc kernel stack
    ptr_t kernel_stack;
    if(!pcb[i].kernel_stack_base){
        // haven't alloc before
        // no page to reuse
        if(!strcmp(tasks[taskid].name,"shell")){
            kernel_stack = allocPage(2) + 2*PAGE_SIZE;
        }else{
            kernel_stack = allocPage(1) + PAGE_SIZE;
        }
    }else{
        // reuse page
        // todo: what if new task is a shell (thus requires more pages)
        kernel_stack = pcb[i].kernel_stack_base;
    }

    // alloc user stack
    ptr_t user_stack;
    user_stack = USER_STACK_ADDR;
    alloc_page_helper(USER_STACK_ADDR-PAGE_SIZE, pcb+i);

    // alloc and load task by reading sd card
    ptr_t entrypoint = load_app_img(taskid, pcb+i);

    pf_list_print(&pcb[i].pf_list);
    printl("\n");
    swp_list_print(&pcb[i].swp_list);
    printl("\n");

    // init pcb stack
    regs_context_t *pt_regs = init_pcb_stack(
        kernel_stack,
        user_stack,
        entrypoint,
        pcb+i
    );
    
    list_push(&ready_queue, &pcb[i].list);

    return pt_regs;
}

// init pcb[i] by loading app named `taskname`
regs_context_t *init_pcb_via_name(int i, char *taskname){
    return init_pcb_via_id(i, find_task_named(taskname));
}

#ifdef S_CORE
pid_t do_exec(int id, int argc, uint64_t arg0, uint64_t arg1, uint64_t arg2){
#else
pid_t do_exec(char *name, int argc, char *argv[]){
#endif
    for(int i=1;i<NUM_MAX_TASK;i++){
        if(pcb[i].status==TASK_UNUSED){

            // save the args in buff
            // in case argv is swapped out of memory in the following init_pcb_via_id
            // note: we assume argc < 100, argv length < 4K
            // todo: what if the args is larger than a page size?
            static char *_argv[100];
            static char _argv_buff[4096];
            int _argv_buff_head = 0;

            for(int j=0;j<argc;j++){
                strcpy(_argv_buff + _argv_buff_head, argv[j]);
                _argv[j] = _argv_buff + _argv_buff_head;
                _argv_buff_head += strlen(argv[j])+1;
            }
            _argv[argc] = (uint64_t)0;


// #ifdef S_CORE
//             uint64_t entrypoint = load_task_img(id);
// #else
//             uint64_t entrypoint = load_task_img_via_name(name);
// #endif
// 
//             if(!entrypoint){
//                 // is bat or no such task
//                 return 0;
//             }

            int taskid = find_task_named(name);
            if(taskid==-1){
                return 0;
            }
            if(tasks[taskid].type==bat){
                // printk("[kernel] Not support to run BAT yet!\n");
                load_bat_img(taskid);
                return 0;
            }

            printl("exec %s\n",name);

#ifdef S_CORE
            regs_context_t *pt_regs = init_pcb_via_id(i, taskid);
            pt_regs->regs[10] = argc;   //a0
            pt_regs->regs[11] = arg0;   //a1
            pt_regs->regs[12] = arg1;   //a2
            pt_regs->regs[13] = arg2;   //a3
#else
            regs_context_t *pt_regs = init_pcb_via_id(i, taskid);
            ptr_t user_sp_origin = pcb[i].user_sp;
            ptr_t user_sp_now = user_sp_origin-8*(argc+1);
            ptr_t argv_base = user_sp_now;

            // todo: what if the args is larger than a page size?

            uint64_t *argv_base_kva = (uint64_t *)(check_and_get_kva_of(argv_base, &pcb[i]));

            for(int j=0;j<argc;j++){
                user_sp_now -= strlen(_argv[j])+1;
                strcpy((char *)get_kva_of(user_sp_now, pcb[i].pgdir), _argv[j]);
                argv_base_kva[j] = user_sp_now;
            }
            argv_base_kva[argc] = (uint64_t)0;

            // alignment
            while(user_sp_now%16){
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

    do_kill(current_running_of[get_current_cpu_id()]->pid);
    do_scheduler();
}

int do_kill(pid_t pid){
    for(int i=1;i<process_id;i++){
        if(pcb[i].status!=TASK_UNUSED){
            if(pcb[i].pid==pid){
                list_delete(&pcb[i].list);

                // wakeup all the node blocked on wait_list
                list_node_t *node;
                while((node=list_pop(&pcb[i].wait_list))){
                    do_unblock(node);
                }

                // todo: release occupied resources (such as threads)
                for(int j=0;j<LOCK_NUM;j++){
                    spin_lock_acquire(&mlocks[j].lock);
                    if(mlocks[j].owner==pid){
                        do_mutex_lock_release_compulsorily(j);
                    }
                    spin_lock_release(&mlocks[j].lock);
                }

                // kill threads
                while((node=list_pop(&pcb[i].tcb_list))){
                    list_delete(&TCBLIST2TCB(node)->list);
                }

                // recycle pages
                while((node=list_pop(&pcb[i].pf_list))){
                    LIST2PF(node)->va = 0;
                    LIST2PF(node)->owner = -1;
                    list_push(&free_pf_pool, node);
                }
                while((node=list_pop(&pcb[i].swp_list))){
                    LIST2SWP(node)->va = 0;
                    LIST2SWP(node)->owner = -1;
                    list_push(&free_swp_pool, node);
                }

                // pcb[i].status=TASK_EXITED;
                pcb[i].status=TASK_UNUSED;
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
                    do_block(&current_running_of[get_current_cpu_id()]->list, &pcb[i].wait_list, &pcb_lock);
                }
                return pid;
            }
        }
    }
    return 0;
}

pid_t do_getpid(){
    return current_running_of[get_current_cpu_id()]->pid;
}

// copied from tiny_libc/itoa.c
static int itoa(int num, char* str, int len, int base)
{
	int sum = num;
	int i = 0;
	int digit;

	if (len == 0)
		return -1;

	do
	{
		digit = sum % base;

		if (digit < 0xA)
			str[i++] = '0' + digit;
		else
			str[i++] = 'A' + digit - 0xA;

		sum /= base;
	} while (sum && (i < (len - 1)));

	if (i == (len - 1) && sum)
		return -1;

	str[i] = '\0';

    int _i, _j;
    int _len = strlen(str);

    for (_i = 0, _j = _len - 1; _i < _j; _i++, _j--)
    {
        char tmp = str[_i];
        str[_i] = str[_j];
        str[_j] = tmp;
    }

	return 0;
}

void do_process_show(){
    printk("[Process Table]\n");
    printk(" IDX PID STATUS MASK TASK_NAME\n");
    for(int i=0;i<process_id;i++){
        if(pcb[i].status!=TASK_UNUSED){
            char mask_str[10];
            itoa(pcb[i].mask, mask_str, 9, 16);
            printk("[%02d]  %02d %s 0x%s %s", 
                i, pcb[i].pid, task_status_str[pcb[i].status], 
                mask_str ,pcb[i].name);
            if(pcb[i].status==TASK_RUNNING){
                printk(" @ running on core%d",pcb[i].running_core);
            }
            printk("\n");
        }
    }
    printk("current ready_queue: ");
    // copied from sched.h
    list_node_t *next=&ready_queue;
    while((next=next->next)!=&ready_queue){
        printk("%d ",LIST2PCB(next)->pid);
    }
    printk("\n");
}

void do_task_show(){
    printk("[Task Table]\n");
    printk(" IDX TYPE LOADED NAME\n");
    for(int i=0;i<task_num;i++){
        printk("[%02d]  %s %s %s\n", 
            i, tasks[i].type==app?"APP":"BAT", tasks[i].loaded?"LOADED":"NOTYET", tasks[i].name);
    }
    // printk("Note: Not supported to run tasks with type BAT yet!\n");
    printk("Note: due to the bug that sub core cannot read sd, \n      there is possibility of malfunctioning if sub core calls BAT\n");
}

int taskset_via_name(int mask, char *name, int argc, char **argv){
    return taskset_via_pid(mask, do_exec(name, argc, argv));   
}

int taskset_via_pid(int mask, pid_t pid){
    for(int i=1;i<NUM_MAX_TASK;i++){
        if(pcb[i].status!=TASK_UNUSED && pcb[i].pid==pid){
            pcb[i].mask=mask;
            return 1;
        }
    }
    return 0;
}