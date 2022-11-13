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

pcb_t *find_father_pcb_for_tcb(tcb_t *tcb){
    list_node_t *father_node = &tcb->tcb_list;
    while(TCBLIST2TCB(father_node)->tid){
        father_node = father_node->next;
    }
    return TCBLIST2TCB(father_node);
}

// for [p2-task5]
extern void ret_from_exception();
void thread_create(tid_t *tidptr, uint64_t entrypoint, long a0, long a1, long a2){
    pcb_t *father_pcb = find_father_pcb_for_tcb(current_running_of[get_current_cpu_id()]);
    list_node_t *father_node = &father_pcb->tcb_list;

    // alloc kernel stack
    // all the page of the new thread is counted on its father
    // set `va` to 0 to prevent the kernel_stack from being swapped out of memory
    ptr_t kernel_stack = alloc_page_from_pool(0, father_pcb) + PAGE_SIZE;

    // alloc tcb in kernel_stack
    kernel_stack -= sizeof(tcb_t); 
    while(kernel_stack%16){ // stack pointer should be 128 bit aligned
        kernel_stack--;
    }
    tcb_t *tcb = (tcb_t *)kernel_stack;

    // init tcb
    assert(father_pcb->pid==current_running_of[get_current_cpu_id()]->pid);
    tcb->pid = current_running_of[get_current_cpu_id()]->pid;
    tcb->status = TASK_READY;
    
    tcb->running_core = -1;
    tcb->mask = current_running_of[get_current_cpu_id()]->mask;

    tcb->tid = TCBLIST2TCB(father_node->prev)->tid + 1;
    tcb->name[0] = '0' + tcb->tid;
    tcb->name[1] = '\0';
    list_push(father_node, &tcb->tcb_list);

    tcb->wait_list.prev = &tcb->wait_list;
    tcb->wait_list.next = &tcb->wait_list;
    
    tcb->pf_list.prev = &tcb->pf_list;
    tcb->pf_list.next = &tcb->pf_list;
    tcb->swp_list.prev = &tcb->swp_list;
    tcb->swp_list.next = &tcb->swp_list;

    assert(father_pcb->pgdir==current_running_of[get_current_cpu_id()]->pgdir);
    tcb->pgdir = current_running_of[get_current_cpu_id()]->pgdir;
    
    tid_t *tidptr_kva = (tid_t *)(get_kva_of((uintptr_t)tidptr, tcb->pgdir));
    if(!tidptr_kva){
        // the page that `tidptr` points to has just been swapped out memory
        // therefore we should swap in first
        list_node_t *swap_node = find_and_pop_swp_node((uintptr_t)tidptr, father_pcb);
        assert(swap_node);
        swap_in(LIST2SWP(swap_node), father_pcb);
    }
    tidptr_kva = (tid_t *)(get_kva_of((uintptr_t)tidptr, tcb->pgdir));
    
    *tidptr_kva=tcb->tid;

    // alloc user stack
    ptr_t user_stack = USER_STACK_ADDR + 0x10000 * tcb->tid;
    // all the page of the new thread is counted on its father
    alloc_page_helper(user_stack-PAGE_SIZE, father_pcb);

    // init tcb stack
    regs_context_t *pt_regs = init_pcb_stack(kernel_stack, user_stack, entrypoint, tcb);
    pt_regs->regs[10]   = (reg_t) a0;
    pt_regs->regs[11]   = (reg_t) a1;
    pt_regs->regs[12]   = (reg_t) a2;
    // pt_regs->regs[13]   = (reg_t) a3;

    list_push(&ready_queue, &tcb->list);
}

void thread_exit(void *retval){
    // todo: call this once thread ret

    current_running_of[get_current_cpu_id()]->status = TASK_EXITED;
    current_running_of[get_current_cpu_id()]->retval = retval;

    // wakeup all the node blocked on wait_list
    list_node_t *node;
    while((node=list_pop(&current_running_of[get_current_cpu_id()]->wait_list))){
        do_unblock(node);
    }

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

    pcb_t *father_pcb = find_father_pcb_for_tcb(current_running_of[get_current_cpu_id()]);
    void **retvalptr_kva = (void **)(get_kva_of((uintptr_t)retvalptr, father_pcb->pgdir));
    if(!retvalptr_kva){
        // the page that `retvalptr` points to has been swapped out memory
        // therefore we should swap in first
        list_node_t *swap_node = find_and_pop_swp_node((uintptr_t)retvalptr, father_pcb);
        assert(swap_node);
        swap_in(LIST2SWP(swap_node), father_pcb);
    }
    retvalptr_kva = (void **)(get_kva_of((uintptr_t)retvalptr, father_pcb->pgdir));
    *retvalptr_kva = joining_tcb->retval;

    list_delete(joining_node);
    // list_pop(joining_node->prev); // delete joining_node from tcb_list

    // the stack pages will be recycled once the father process is killed or exited

    return 0;
}
