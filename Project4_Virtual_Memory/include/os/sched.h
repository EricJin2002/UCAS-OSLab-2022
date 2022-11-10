/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *        Process scheduling related content, such as: scheduler, process blocking,
 *                 process wakeup, process creation, process kill, etc.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
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

#ifndef INCLUDE_SCHEDULER_H_
#define INCLUDE_SCHEDULER_H_

#include <type.h>
#include <os/list.h>
#include <os/lock.h> // for [p3]
#include <os/smp.h>  // for [p3]

#define NUM_MAX_TASK 30

/* used to save register infomation */
typedef struct regs_context
{
    /* Saved main processor registers.*/
    reg_t regs[32];

    /* Saved special registers. */
    reg_t sstatus;
    reg_t sepc;
    reg_t sbadaddr;
    reg_t scause;
} regs_context_t;

/* used to save register infomation in switch_to */
typedef struct switchto_context
{
    /* Callee saved registers.*/
    reg_t regs[14];
} switchto_context_t;

typedef enum {
    TASK_BLOCKED,
    TASK_RUNNING,
    TASK_READY,
    TASK_EXITED,
    TASK_UNUSED
} task_status_t;

// for [p3-task1]
extern const char *task_status_str[];

/* Process Control Block */
typedef struct pcb
{
    /* register context */
    // NOTE: this order must be preserved, which is defined in regs.h!!
    reg_t kernel_sp;
    reg_t user_sp;
    ptr_t kernel_stack_base;
    ptr_t user_stack_base;

    /* previous, next pointer */
    list_node_t list;
    list_head wait_list;

    // for [p4]
    list_head pf_list;

    /* pgdir */
    uintptr_t pgdir; // stored as kva

    /* process id */
    pid_t pid;

    // for [p3-task1]
    char name[32];

    // for [p3-task3] & [p3-task4]
    int running_core;
    int mask;

    // for [p2-task5]
    tid_t tid; // 0 indicates main thread
    list_node_t tcb_list;
    void *retval;

    /* BLOCK | READY | RUNNING */
    task_status_t status;

    /* cursor position */
    int cursor_x;
    int cursor_y;

    /* time(seconds) to wake up sleeping PCB */
    uint64_t wakeup_time;
} pcb_t, tcb_t;

/* ready queue to run */
extern list_head ready_queue;

/* sleep queue to be blocked in */
extern list_head sleep_queue;

// for [p3-task3]
pcb_t * volatile current_running_of[NR_CPUS];
pcb_t * volatile kernel_pcb_of[NR_CPUS];

/* current running task PCB */
// extern pcb_t * volatile current_running;
extern pid_t process_id;

extern pcb_t pcb[NUM_MAX_TASK];
extern pcb_t pid0_pcb;
extern const ptr_t pid0_stack;

// for [p3]
extern spin_lock_t pcb_lock;

extern void switch_to(pcb_t *prev, pcb_t *next);
void do_scheduler(void);
void do_sleep(uint32_t);

void do_block(list_node_t *, list_head *queue, spin_lock_t *lock);
void do_unblock(list_node_t *);

// for [p2-task1]
// #define LIST2PCB(listptr) ((pcb_t *)((void *)(listptr)-16)) // wrong in [p3]
#define LIST2PCB(listptr) ((pcb_t *)((void *)(listptr)-STRUCT_OFFSET(pcb, list)))
// for debug
#include <printk.h>
static inline void pcb_list_print(list_head *listptr){
    list_node_t *next=listptr;
    while((next=next->next)!=listptr){
        printl("%d ",LIST2PCB(next)->pid);
    }
    printl("\n\r");
}

// for [p2-task5]
// #define TCBLIST2TCB(listptr) ((tcb_t *)((void *)(listptr)-40)) // wrong in [p3]
#define TCBLIST2TCB(listptr) ((tcb_t *)((void *)(listptr)-STRUCT_OFFSET(pcb, tcb_list)))

// for [p2-task5]
void thread_create(tid_t *tidptr, uint64_t entrypoint, long a0, long a1, long a2);
void thread_exit(void *retval);
int thread_join(tid_t tid, void **retvalptr);

// #define S_CORE

/* TODO [P3-TASK1] exec exit kill waitpid ps*/
#ifdef S_CORE
extern pid_t do_exec(int id, int argc, uint64_t arg0, uint64_t arg1, uint64_t arg2);
#else
extern pid_t do_exec(char *name, int argc, char *argv[]);
#endif
extern void do_exit(void);
extern int do_kill(pid_t pid);
extern int do_waitpid(pid_t pid);
extern void do_process_show();
extern pid_t do_getpid();

// for [p3-task1]
extern void do_task_show();

// for [p3-task1]
extern regs_context_t *init_pcb_via_id(int i, int taskid);
extern regs_context_t *init_pcb_via_name(int i, char *taskname);

// for [p3-task3]
extern int taskset_via_name(int mask, char *name, int argc, char **argv);
extern int taskset_via_pid(int mask, pid_t pid);

#endif
