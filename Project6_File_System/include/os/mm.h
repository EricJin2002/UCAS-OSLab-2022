/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                                   Memory Management
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
#ifndef MM_H
#define MM_H

#include <type.h>
#include <pgtable.h>
#include <os/list.h>    // for [p4]
#include <os/sched.h>   // for [p4]
#include <printk.h>     // for [p4]

#define MAP_KERNEL 1
#define MAP_USER 2
#define MEM_SIZE 32
#define PAGE_SIZE 4096 // 4K
// #define INIT_KERNEL_STACK 0x50500000
// #define INIT_USER_STACK 0x52500000
#define INIT_KERNEL_STACK 0xffffffc052000000
#define FREEMEM_KERNEL (INIT_KERNEL_STACK+PAGE_SIZE)
// #define FREEMEM_USER INIT_USER_STACK

/* Rounding; only works for n = power of two */
#define ROUND(a, n)     (((((uint64_t)(a))+(n)-1)) & ~((n)-1))
#define ROUNDDOWN(a, n) (((uint64_t)(a)) & ~((n)-1))

// extern ptr_t allocKernelPage(int numPage);
// extern ptr_t allocUserPage(int numPage);


// for [p4]
#define NUM_MAX_PAGEFRAME 100
typedef struct pf{  // page frame
    uint64_t kva;   // kva is fixed
    uint64_t va;    // only non-zero for swappable pages
    pid_t owner;
    pcb_t *owner_pcb;
    list_node_t list;
} pf_t;
extern pf_t pfs[NUM_MAX_PAGEFRAME];
extern list_head free_pf_pool;
#define LIST2PF(listptr) ((pf_t *)((void *)(listptr)-STRUCT_OFFSET(pf, list)))
// for debug
static inline void pf_list_print(list_head *listptr){
    list_node_t *next=listptr;
    while((next=next->next)!=listptr){
        printl("kva 0x%x ",LIST2PF(next)->kva);
        printl("owener %d ",LIST2PF(next)->owner);
        printl("va 0x%lx ",LIST2PF(next)->va);
        printl("\n");
    }
    // printl("\n\r");
}

// for [p4-task3]
# define NUM_MAX_SWAPPAGE 400
typedef struct swp{
    int block_id;
    uint64_t va;
    pid_t owner;
    list_node_t list;
} swp_t;
extern swp_t swps[NUM_MAX_SWAPPAGE];
extern list_head free_swp_pool;
#define LIST2SWP(listptr) ((swp_t *)((void *)(listptr)-STRUCT_OFFSET(swp, list)))
// for debug
static inline void swp_list_print(list_head *listptr){
    list_node_t *next=listptr;
    while((next=next->next)!=listptr){
        printl("block id %d ",LIST2SWP(next)->block_id);
        printl("owener %d ",LIST2SWP(next)->owner);
        printl("va 0x%lx ",LIST2SWP(next)->va);
        printl("\n");
    }
    // printl("\n\r");
}
extern int swap_start_sector_id;
extern int swap_end_sector_id;

// for [p4-task5]
# define NUM_MAX_SHMPAGE 20
typedef struct shm{
    pf_t *pf;   // pf->va == 0, which indicates unswappable
    int key;
    int handle_num;
} shm_t;
extern shm_t shms[NUM_MAX_SHMPAGE];
#define SHMPAGE_VA_BASE 0x10000000
#define SHMPAGE_VA_SIZE 0x10000000

// for [p4]
extern void init_pages();
extern void swap_out(int pf_id);
extern void swap_out_randomly();
extern void swap_in(swp_t *swpptr, pcb_t *owner_pcb);
extern list_node_t *find_and_pop_swp_node(uintptr_t va, pcb_t *owner_pcb);
extern list_node_t *find_pf_node(uintptr_t va, pcb_t *owner_pcb);
extern ptr_t alloc_page_from_pool(uintptr_t va, pcb_t *owner_pcb);
extern uintptr_t check_and_get_kva_of(uintptr_t va, pcb_t *owner_pcb);

extern ptr_t allocPage(int numPage);
// TODO [P4-task1] */
void freePage(ptr_t baseAddr);

// #define S_CORE
// NOTE: only need for S-core to alloc 2MB large page
#ifdef S_CORE
#define LARGE_PAGE_FREEMEM 0xffffffc056000000
#define USER_STACK_ADDR 0x400000
extern ptr_t allocLargePage(int numPage);
#else
// NOTE: A/C-core
#define USER_STACK_ADDR 0xf00010000
#endif

// TODO [P4-task1] */
extern void* kmalloc(size_t size);
extern void share_pgtable(uintptr_t dest_pgdir, uintptr_t src_pgdir);
extern uintptr_t alloc_page_helper(uintptr_t va, /*uintptr_t pgdir*/pcb_t *owner_pcb);
extern void map_page(uintptr_t va, uintptr_t pa, pcb_t *owner_pcb);
extern void map_page_2(uintptr_t va, uintptr_t pa, PTE *pgdir);

// TODO [P4-task4]: shm_page_get/dt */
uintptr_t shm_page_get(int key);
void shm_page_dt(uintptr_t addr);

// for [p4-task6]
uintptr_t get_pa_of(uintptr_t va);
uintptr_t take_snapshot(uintptr_t pgdir_va);
#define SNAPSHOT_VA_BASE 0x20000000
#define SNAPSHOT_VA_SIZE 0x10000000

#endif /* MM_H */
