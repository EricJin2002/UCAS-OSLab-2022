/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *         The kernel's entry, where most of the initialization work is done.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#include <common.h>
#include <asm.h>
#include <asm/unistd.h>
#include <os/loader.h>
#include <os/irq.h>
#include <os/sched.h>
#include <os/lock.h>
#include <os/kernel.h>
#include <os/task.h>
#include <os/string.h>
#include <os/mm.h>
#include <os/time.h>
#include <sys/syscall.h>
#include <screen.h>
#include <printk.h>
#include <assert.h>
#include <type.h>
#include <csr.h>
#include <os/smp.h>
#include <pgtable.h>    // for [p4-task1]

extern void ret_from_exception();

// Task info array
task_info_t tasks[TASK_MAXNUM];

// for [p1-task4]
uint16_t task_num;

// #define TASK_NAME_MAXLEN 20

static void init_jmptab(void)
{
    volatile long (*(*jmptab))() = (volatile long (*(*))())KERNEL_JMPTAB_BASE;

    jmptab[CONSOLE_PUTSTR]  = (long (*)())port_write;
    jmptab[CONSOLE_PUTCHAR] = (long (*)())port_write_ch;
    jmptab[CONSOLE_GETCHAR] = (long (*)())port_read_ch;
    jmptab[SD_READ]         = (long (*)())sd_read;
    jmptab[SD_WRITE]        = (long (*)())sd_write;
    jmptab[QEMU_LOGGING]    = (long (*)())qemu_logging;
    jmptab[SET_TIMER]       = (long (*)())set_timer;
    jmptab[READ_FDT]        = (long (*)())read_fdt;
    jmptab[MOVE_CURSOR]     = (long (*)())screen_move_cursor;
    jmptab[PRINT]           = (long (*)())printk;
    jmptab[YIELD]           = (long (*)())do_scheduler;
    jmptab[MUTEX_INIT]      = (long (*)())do_mutex_lock_init;
    jmptab[MUTEX_ACQ]       = (long (*)())do_mutex_lock_acquire;
    jmptab[MUTEX_RELEASE]   = (long (*)())do_mutex_lock_release;
}

static void init_task_info(void)
{
    // TODO: [p1-task4] Init 'tasks' array via reading app-info sector
    // NOTE: You need to get some related arguments from bootblock first
    uint64_t task_info_off = *(uint32_t *)(pa2kva(0x50200200 - 0xc));
    task_info_t *tasks_mem_ptr = (task_info_t *)(pa2kva(0x52000000 + task_info_off%0x200));
    // copy task_info from mem to bss
    // since mem will be overwritten by the first app
    memcpy((uint8_t *)tasks, (uint8_t *)tasks_mem_ptr, TASK_MAXNUM * sizeof(task_info_t));

    // for [p1-task3] & [p1-task4]
    // read task_num
    task_num = *(uint16_t *)(pa2kva(0x502001fe));

    for(int i=0; i<task_num; i++){
        printl("task name %s entrypoint %lx\n", tasks[i].name, tasks[i].entrypoint);
    }
}

// init_pcb_stack moved to sched.c

static void init_pcb(void)
{
    /* TODO: [p2-task1] load needed tasks and init their corresponding PCB */
    // for [p2-task1]
    //char needed_task_name[][32] = {"print1", "print2", "fly"};

    // for [p2-task2]
    //char needed_task_name[][32] = {"print1", "print2", "fly", "lock1", "lock2", "timer", "sleep", "add2"};

    // for [p3-task1]
    // `ps` won't print exited pcb
    for(int i=0;i<NUM_MAX_TASK;i++){
        pcb[i].status=TASK_UNUSED;
        pcb[i].kernel_stack_base=0;
        pcb[i].user_stack_base=0;
        pcb[i].pgdir=0;
        pcb[i].wait_list.prev = &pcb[i].wait_list;
        pcb[i].wait_list.next = &pcb[i].wait_list;
        pcb[i].pf_list.prev = &pcb[i].pf_list;
        pcb[i].pf_list.next = &pcb[i].pf_list;
        pcb[i].swp_list.prev = &pcb[i].swp_list;
        pcb[i].swp_list.next = &pcb[i].swp_list;
    }

    /* TODO: [p2-task1] remember to initialize 'current_running_of[0]' */

    pid0_pcb.pid = 0;
    pid0_pcb.kernel_sp = (ptr_t)pid0_stack;
    pid0_pcb.user_sp = (ptr_t)pid0_stack;
    strcpy(pid0_pcb.name,"core0");
    pid0_pcb.mask = 3;
    pid0_pcb.list.prev=&pid0_pcb.list;
    pid0_pcb.list.next=&pid0_pcb.list;
    pid0_pcb.wait_list.prev=&pid0_pcb.wait_list;
    pid0_pcb.wait_list.next=&pid0_pcb.wait_list;
    pid0_pcb.tid=0;
    pid0_pcb.tcb_list.prev=&pid0_pcb.tcb_list;
    pid0_pcb.tcb_list.next=&pid0_pcb.tcb_list;
    pid0_pcb.pgdir=pa2kva(PGDIR_PA);
    
    // current_running_of[0]->status=TASK_BLOCKED; // to stop pcb0 from being pushed into ready_queue
    pid0_pcb.status=TASK_RUNNING; // the logic above is moved into do_scheduler in [p3-task4]
    pid0_pcb.running_core=0;

    current_running_of[0]=&pid0_pcb;

    // for [p2-task4]
    asm volatile("mv tp, %0"::"r"(current_running_of[0]));

    // for [p3]
    kernel_pcb_of[0]=&pid0_pcb;

    // for [p3-task1]
    char needed_task_name[][32] = {"shell"};

    for(int i=1; i<=sizeof(needed_task_name)/32; i++){
        init_pcb_via_name(i, needed_task_name[i-1]);
    }

    printl("initial ready_queue ");
    pcb_list_print(&ready_queue);
}

static void init_syscall(void)
{
    // TODO: [p2-task3] initialize system call table.
    syscall[SYSCALL_SLEEP]          = (long (*)())do_sleep;
    syscall[SYSCALL_YIELD]          = (long (*)())do_scheduler;
    syscall[SYSCALL_WRITE]          = (long (*)())screen_write;
    syscall[SYSCALL_CURSOR]         = (long (*)())screen_move_cursor;
    syscall[SYSCALL_REFLUSH]        = (long (*)())screen_reflush;
    syscall[SYSCALL_CLEAR]          = (long (*)())screen_clear;
    syscall[SYSCALL_BACKSPACE]      = (long (*)())screen_backspace;
    
    syscall[SYSCALL_GET_TIMEBASE]   = (long (*)())get_time_base;
    syscall[SYSCALL_GET_TICK]       = (long (*)())get_ticks;
    syscall[SYSCALL_LOCK_INIT]      = (long (*)())do_mutex_lock_init;
    syscall[SYSCALL_LOCK_ACQ]       = (long (*)())do_mutex_lock_acquire;
    syscall[SYSCALL_LOCK_RELEASE]   = (long (*)())do_mutex_lock_release;
    
    syscall[SYSCALL_THREAD_CREATE]  = (long (*)())thread_create;
    syscall[SYSCALL_THREAD_EXIT]    = (long (*)())thread_exit;
    syscall[SYSCALL_THREAD_JOIN]    = (long (*)())thread_join;

    syscall[SYSCALL_PS]             = (long (*)())do_process_show;
    syscall[SYSCALL_READCH]         = (long (*)())bios_getchar;
    syscall[SYSCALL_EXEC]           = (long (*)())do_exec;
    syscall[SYSCALL_EXIT]           = (long (*)())do_exit;
    syscall[SYSCALL_KILL]           = (long (*)())do_kill;
    syscall[SYSCALL_WAITPID]        = (long (*)())do_waitpid;
    syscall[SYSCALL_GETPID]         = (long (*)())do_getpid;

    syscall[SYSCALL_SHOW_TASK]      = (long (*)())do_task_show;

    syscall[SYSCALL_BARR_INIT]      = (long (*)())do_barrier_init;
    syscall[SYSCALL_BARR_WAIT]      = (long (*)())do_barrier_wait;
    syscall[SYSCALL_BARR_DESTROY]   = (long (*)())do_barrier_destroy;

    syscall[SYSCALL_COND_INIT]      = (long (*)())do_condition_init;
    syscall[SYSCALL_COND_WAIT]      = (long (*)())do_condition_wait;
    syscall[SYSCALL_COND_SIGNAL]    = (long (*)())do_condition_signal;
    syscall[SYSCALL_COND_BROADCAST] = (long (*)())do_condition_broadcast;
    syscall[SYSCALL_COND_DESTROY]   = (long (*)())do_condition_destroy;

    syscall[SYSCALL_MBOX_OPEN]      = (long (*)())do_mbox_open;
    syscall[SYSCALL_MBOX_CLOSE]     = (long (*)())do_mbox_close;
    syscall[SYSCALL_MBOX_RECV]      = (long (*)())do_mbox_recv;
    syscall[SYSCALL_MBOX_SEND]      = (long (*)())do_mbox_send;
    
    syscall[SYSCALL_TASKSET_NAME]   = (long (*)())taskset_via_name;
    syscall[SYSCALL_TASKSET_PID]    = (long (*)())taskset_via_pid;

    syscall[SYSCALL_SHM_GET]        = (long (*)())shm_page_get;
    syscall[SYSCALL_SHM_DT]         = (long (*)())shm_page_dt;
}

// for [p3-task3]
// static void load_all_tasks_in_advance(void){
//     // due to the bug that subcore cannot sd_read
//     // we let main core to load all tasks in advance
// 
//     for(int i=0;i<task_num;i++){
//         if(tasks[i].type==app){
//             // bat will be executed once loaded, thus we only load app here
//             load_task_img(i);
//         }
//     }
// }

extern void clear_SIP();
int main(void)
{
    if(get_current_cpu_id()!=0){

        lock_kernel();
        ptr_t kernel_stack = allocPage(1)+PAGE_SIZE;
        unlock_kernel();

        kernel_stack-=sizeof(pcb_t);
        // 128 bit aligned
        while(kernel_stack%16){
            kernel_stack--;
        }
        pcb_t *pcb_for_new_core = (pcb_t *)kernel_stack;

        pcb_for_new_core->pid=0;
        char name[]="core0";
        name[4]+=get_current_cpu_id(); // note: it implies core number < 10
        strcpy(pcb_for_new_core->name,name);
        pcb_for_new_core->kernel_sp=kernel_stack;
        pcb_for_new_core->user_sp=kernel_stack;
        pcb_for_new_core->cursor_x=0;
        pcb_for_new_core->cursor_y=4+get_current_cpu_id();
        pcb_for_new_core->status=TASK_RUNNING;
        pcb_for_new_core->list.prev=&pcb_for_new_core->list;
        pcb_for_new_core->list.next=&pcb_for_new_core->list;
        pcb_for_new_core->wait_list.prev=&pcb_for_new_core->wait_list;
        pcb_for_new_core->wait_list.next=&pcb_for_new_core->wait_list;
        pcb_for_new_core->tid=0;
        pcb_for_new_core->tcb_list.prev=&pcb_for_new_core->tcb_list;
        pcb_for_new_core->tcb_list.next=&pcb_for_new_core->tcb_list;
        pcb_for_new_core->mask=3;
        pcb_for_new_core->running_core=get_current_cpu_id();
        pcb_for_new_core->pgdir=pa2kva(PGDIR_PA);

        asm volatile("mv tp, %0":"=r"(pcb_for_new_core));
        current_running_of[get_current_cpu_id()]=pcb_for_new_core;
        kernel_pcb_of[get_current_cpu_id()]=pcb_for_new_core;

        printk("Successfully aroused (sub core)! [%s]\n",name);

        setup_exception();

        bios_set_timer(get_ticks() + TIMER_INTERVAL);
        enable_interrupt();

        while(1){
            enable_preempt();
            // printl("core%d waiting for tasks!\n",get_current_cpu_id());
            asm volatile("wfi");
        }
    }

    // Init jump table provided by kernel and bios(ΦωΦ)
    init_jmptab();

    // Init task information (〃'▽'〃)
    init_task_info();

    // for [p3-task3]
    // load_all_tasks_in_advance();

    // for [p4-task3]
    init_pages();

    // Init Process Control Blocks |•'-'•) ✧
    init_pcb();
    printk("> [INIT] PCB initialization succeeded.\n");

    // Read CPU frequency (｡•ᴗ-)_
    time_base = bios_read_fdt(TIMEBASE);
    printl("time_base %u\n", time_base);

    // Init lock mechanism o(´^｀)o
    init_locks();
    printk("> [INIT] Lock mechanism initialization succeeded.\n");

    // for [p3-task2]
    // Init barrier mechanism
    init_barriers();
    printk("> [INIT] Barrier mechanism initialization succeeded.\n");

    // for [p3-task2]
    // Init condition mechanism
    init_conditions();
    printk("> [INIT] Condition mechanism initialization succeeded.\n");

    // for [p3-task2]
    // Init mailbox mechanism
    init_mbox();
    printk("> [INIT] Mailbox mechanism initialization succeeded.\n");

    // Init interrupt (^_^)
    init_exception();
    printk("> [INIT] Interrupt processing initialization succeeded.\n");

    // Init system call table (0_0)
    init_syscall();
    printk("> [INIT] System call initialized successfully.\n");

    // Init screen (QAQ)
    init_screen();
    printk("> [INIT] SCREEN initialization succeeded.\n");

    // for [p3-task3]
    smp_init();
    wakeup_other_hart();
    clear_SIP(); // main core will also receive ipi, thus we clear sip here

    // TODO: [p2-task4] Setup timer interrupt and enable all interrupt globally
    // NOTE: The function of sstatus.sie is different from sie's
    bios_set_timer(get_ticks() + TIMER_INTERVAL);
    enable_interrupt();

    // bios_putstr("\n\r");

    // for [p1-task2]
    /* int ch;
     * while((ch=bios_getchar())){
     *     if(ch!=-1){
     *         if(ch=='\r'){
     *             // \r for Carriage Return and \n for Line Feed
     *             bios_putstr("\n\r");
     *         }else
     *             bios_putchar(ch);
     *     }
     * }
     */
    

    // load and excute tasks by id for [p1-task3]
    /* int ch;
     * while((ch=bios_getchar())){
     *     if(ch!=-1){
     *         bios_putchar(ch);
     *         bios_putstr("\n\r");
     *         if(ch<'0'+task_num && ch>='0'){
     *             ((void (*)())load_task_img(ch-'0'))();
     *         }else{
     *             bios_putstr("Invalid task id!\n\r");
     *         }
     *     }
     * }
     */


    // abandoned implementation
/*    // load and excute batch for [p1-task5]
 *
 *    bios_putstr("Reading batch from image...\n\r======================\n\r");
 *
 *    // read batch size
 *    uint32_t task_info_off = *(uint32_t *)(0x50200200 - 0xc);
 *    uint32_t bat_size_off = task_info_off + sizeof(task_info_t)*TASK_MAXNUM;
 *    uint16_t bat_size_block_id = bat_size_off/SECTOR_SIZE;
 *    uint16_t bat_size_block_num = NBYTES2SEC(bat_size_off%SECTOR_SIZE + 4);
 *    bios_sdread(0x52000000, bat_size_block_num, bat_size_block_id);
 *    uint32_t bat_size = *(uint32_t *)(0x52000000 + bat_size_off%SECTOR_SIZE);
 *    
 *    // read batch content
 *    char bat_cache[1024]; //TODO: what if bat.txt is too big
 *    uint32_t bat_off = bat_size_off + 4;
 *    uint16_t bat_block_id = bat_off/SECTOR_SIZE;
 *    uint16_t bat_block_num = NBYTES2SEC(bat_off%SECTOR_SIZE + bat_size);
 *    bios_sdread(0x52000000, bat_block_num, bat_block_id);
 *    memcpy(bat_cache, 0x52000000 + bat_off%SECTOR_SIZE, bat_size);
 *    bios_putstr(bat_cache);
 *    bios_putstr("\n\r======================\n\rFinish reading!\n\r");
 *
 *    bios_putstr("\n\r");
 *
 *    //excute batch
 *    bios_putstr("\n\rNow excute batch!\n\r======================\n\r");
 *    int bat_iter = 0;
 *    int bat_iter_his = 0;
 *    while(bat_cache[bat_iter]){
 *        if(bat_cache[bat_iter]=='\n'){
 *            bat_cache[bat_iter]='\0';
 *            excute_task_img_via_name(bat_cache + bat_iter_his);
 *            bat_iter_his=bat_iter+1;
 *        }
 *        bat_iter++;
 *    }
 *    excute_task_img_via_name(bat_cache + bat_iter_his);
 *    bios_putstr("======================\n\rAll tasks in batch are excuted!\n\r");
 *
 *    bios_putstr("\n\r");
 */

    // load and excute tasks by name for [p1-task4]
/*    bios_putstr("Type help to show help infomation\n\r");
 *    bios_putstr("Type quit to skip\n\r");
 *    char cache[TASK_NAME_MAXLEN+1];
 *    int head=0;
 *    bios_putstr("$ ");
 *    int ch;
 *    while((ch=bios_getchar())){
 *        if(ch!=-1){
 *            if(ch=='\r'){// \r for Carriage Return and \n for Line Feed
 *                bios_putstr("\n\r");
 *                cache[head]='\0';
 *                if(!strcmp(cache, "quit"))
 *                    break;
 *                excute_task_img_via_name(cache);
 *                head=0;
 *                bios_putstr("$ ");
 *            }else{
 *                if(head<TASK_NAME_MAXLEN){
 *                    bios_putchar(ch);
 *                    cache[head++]=ch;
 *                }else{
 *                    bios_putstr("\n\rMaximal input reached!\n\r");
 *                    head=0;
 *                    bios_putstr("$ ");
 *                }
 *            }
 *        }
 *    }
 *
 *    init_screen();
 */

    // Infinite while loop, where CPU stays in a low-power state (QAQQQQQQQQQQQ)
    while (1)
    {
        // If you do non-preemptive scheduling, it's used to surrender control
        // do_scheduler();

        // If you do preemptive scheduling, they're used to enable CSR_SIE and wfi
        enable_preempt();
        printl("core0 waiting for tasks!\n");
        asm volatile("wfi");
    }

    return 0;
}
