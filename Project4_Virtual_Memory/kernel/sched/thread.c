#include <os/list.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/time.h>
#include <os/mm.h>
#include <screen.h>
#include <printk.h>
#include <assert.h>
#include <csr.h>        // for [p2-task5]
#include <os/smp.h>     // for [p3]

// for [p2-task5]
extern void ret_from_exception();
void thread_create(tid_t *tidptr, uint64_t entrypoint, long a0, long a1, long a2){
    // int i = process_id++;
    // pcb[i].pid = i;
    // pcb[i].status = TASK_READY;

    // todo: alloc stack page for thread

    ptr_t kernel_stack;// = allocKernelPage(1) + PAGE_SIZE;
    ptr_t user_stack;// = allocUserPage(1) + PAGE_SIZE;

    tcb_t *tcb = (tcb_t *)kernel_stack;
    kernel_stack -= sizeof(tcb_t); // todo: stack pointer should be 128 bit aligned
    tcb->pid = current_running_of[get_current_cpu_id()]->pid;
    list_node_t *father_node = &current_running_of[get_current_cpu_id()]->tcb_list;
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
    current_running_of[get_current_cpu_id()]->status = TASK_EXITED;
    current_running_of[get_current_cpu_id()]->retval = retval;
    do_scheduler();
}

// on success, return 0; on error, return -1
int thread_join(tid_t tid, void **retvalptr){
    list_node_t *joining_node = &current_running_of[get_current_cpu_id()]->tcb_list;
    do{
        joining_node = joining_node->next;
        if(joining_node==&current_running_of[get_current_cpu_id()]->tcb_list){
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
