#include <os/mm.h>
#include <assert.h>
#include <os/kernel.h>  // for [p4-task3]
#include <os/loader.h>  // for [p4-task3]
#include <os/task.h>    // for [p4-task3]
#include <os/ioremap.h> // for [p5-task1]

// static ptr_t kernMemCurr = FREEMEM_KERNEL;
// static ptr_t userMemCurr = FREEMEM_USER;
// 
// ptr_t allocKernelPage(int numPage)
// {
//     // align PAGE_SIZE
//     ptr_t ret = ROUND(kernMemCurr, PAGE_SIZE);
//     kernMemCurr = ret + numPage * PAGE_SIZE;
//     return ret;
// }
// 
// ptr_t allocUserPage(int numPage)
// {
//     // align PAGE_SIZE
//     ptr_t ret = ROUND(userMemCurr, PAGE_SIZE);
//     userMemCurr = ret + numPage * PAGE_SIZE;
//     return ret;
// }


// NOTE: A/C-core
static ptr_t kernMemCurr = FREEMEM_KERNEL;

ptr_t allocPage(int numPage)
{
    // align PAGE_SIZE
    ptr_t ret = ROUND(kernMemCurr, PAGE_SIZE);
    kernMemCurr = ret + numPage * PAGE_SIZE;
    return ret;
}

// for [p4]
LIST_HEAD(free_pf_pool);
pf_t pfs[NUM_MAX_PAGEFRAME];

// for [p4-task3]
LIST_HEAD(free_swp_pool);
swp_t swps[NUM_MAX_SWAPPAGE];

// for [p6-task1]
int swap_start_sector_id;
int swap_end_sector_id;

void init_pages(){
    // init page frames
    for(int i=0;i<NUM_MAX_PAGEFRAME;i++){
        pfs[i].kva = allocPage(1);
        pfs[i].va = 0;
        pfs[i].owner = -1;
        list_push(&free_pf_pool, &pfs[i].list);
    }

    // init swap pages
    // the below line is abandoned
    // because it will overwrite task-info area
    // thus cannot repeatedly `make run`
    // int cnt = (tasks[task_num-1].offset + tasks[task_num-1].size + SECTOR_SIZE - 1) / SECTOR_SIZE;

    // instead, we use this
    uintptr_t bootblock_end_addr = 0x50200200;
    uint16_t task_info_block_id = *(uint16_t *)(bootblock_end_addr - 8);
    uint16_t task_info_block_num = *(uint16_t *)(bootblock_end_addr - 6);
    int cnt = (int)task_info_block_id + (int)task_info_block_num;

    swap_start_sector_id=cnt;

    for(int i=0;i<NUM_MAX_SWAPPAGE;i++){
        swps[i].block_id = cnt;
        cnt += PAGE_SIZE/SECTOR_SIZE;
        swps[i].va = 0;
        swps[i].owner = -1;
        list_push(&free_swp_pool, &swps[i].list);
    }

    swap_end_sector_id=cnt;
    printl("swap_start_sector_id %d\n", swap_start_sector_id);
    printl("swap_end_sector_id %d\n",swap_end_sector_id);

    // init shared memory pages
    for(int i=0;i<NUM_MAX_SHMPAGE;i++){
        shms[i].handle_num = 0;
    }
}

void swap_out(int pf_id){
    printl("[in swap_out]\n");
    
    list_node_t *new_swp_node = list_pop(&free_swp_pool);
    assert(new_swp_node);

    swp_t *new_swp = LIST2SWP(new_swp_node);
    assert(new_swp->block_id);
    new_swp->va = pfs[pf_id].va;
    new_swp->owner = pfs[pf_id].owner;
    list_push(&pfs[pf_id].owner_pcb->swp_list, &new_swp->list);

    bios_sdwrite(kva2pa(pfs[pf_id].kva), PAGE_SIZE/SECTOR_SIZE, new_swp->block_id);

    printl("[va 0x%lx owener %d] kva 0x%x is swapped into block_id %d\n", 
        pfs[pf_id].va, pfs[pf_id].owner, pfs[pf_id].kva, new_swp->block_id);

    set_pte_invalid(pfs[pf_id].va, pfs[pf_id].owner_pcb->pgdir);

    pfs[pf_id].va = 0;
    pfs[pf_id].owner = -1;
    list_delete(&pfs[pf_id].list);
    list_push(&free_pf_pool, &pfs[pf_id].list);

    printl("[leave swap_out]\n");
}

void swap_out_randomly(){
    // randomly select page to swap out
    // copied from tiny_libc/rand.c
    static int x = 2022; // seed
    uint64_t tmp = 0x5deece66dll * x + 0xbll;
    x = tmp & 0x7fffffff;
    
    int lucky_pf_id = x%NUM_MAX_PAGEFRAME;
    int found = 0;
    do{
        if(pfs[lucky_pf_id].va){
            // the page is not a pgdir
            // thus swappable
            found=1;
            break;
        }
        lucky_pf_id++;
        lucky_pf_id%=NUM_MAX_PAGEFRAME;
    }while(lucky_pf_id!=x%NUM_MAX_PAGEFRAME);

    if(!found){
        // no available page to swap
        assert(0);  // temporarily
    }

    swap_out(lucky_pf_id);
}

void swap_in(swp_t *swpptr, pcb_t *owner_pcb){
    printl("[in swap_in]\n");

    ptr_t new_pf_kva = alloc_page_helper(swpptr->va, owner_pcb);

    bios_sdread(kva2pa(new_pf_kva), PAGE_SIZE/SECTOR_SIZE, swpptr->block_id);

    printl("[va 0x%lx owener %d] block_id %d is swapped into kva 0x%x\n", 
        swpptr->va, swpptr->owner, swpptr->block_id, new_pf_kva);

    swpptr->va = 0;
    swpptr->owner = -1;
    list_delete(&swpptr->list);
    list_push(&free_swp_pool, &swpptr->list);
    
    printl("[leave swap_in]\n");
}

static uint64_t filter_swp_stval;
static pid_t filter_swp_owner_pid;
static int filter_swp(list_node_t *node){
    swp_t *swpptr = LIST2SWP(node);
    return swpptr->owner == filter_swp_owner_pid 
        && (swpptr->va >> NORMAL_PAGE_SHIFT) == (filter_swp_stval >> NORMAL_PAGE_SHIFT);
}
list_node_t *find_and_pop_swp_node(uintptr_t va, pcb_t *owner_pcb){
    filter_swp_stval = va;
    filter_swp_owner_pid = owner_pcb->pid;
    return list_find_and_pop(&owner_pcb->swp_list, filter_swp);
}

static uint64_t filter_pf_stval;
static pid_t filter_pf_owner_pid;
static int filter_pf(list_node_t *node){
    pf_t *pfptr = LIST2PF(node);
    return pfptr->owner == filter_pf_stval
        && (pfptr->va >> NORMAL_PAGE_SHIFT) == (filter_pf_stval >> NORMAL_PAGE_SHIFT);
}
list_node_t *find_pf_node(uintptr_t va, pcb_t *owner_pcb){
    filter_pf_stval = va;
    filter_pf_owner_pid = owner_pcb->pid;
    return list_find(&owner_pcb->pf_list, filter_pf);
}

ptr_t alloc_page_from_pool(uintptr_t va, pcb_t *owner_pcb){
    if(list_is_empty(&free_pf_pool)){
        swap_out_randomly();
    }
    pf_t *new_pf = LIST2PF(list_pop(&free_pf_pool));
    new_pf->va = va&~(NORMAL_PAGE_SIZE-1);
    new_pf->owner = owner_pcb->pid;
    new_pf->owner_pcb = owner_pcb;
    list_push(&owner_pcb->pf_list, &new_pf->list);
    // pf_list_print(&owner_pcb->pf_list);
    // printl("\n");
    return new_pf->kva;
}

// the data might have been swapped out of memory, thus we check it and swap in if necessary
uintptr_t check_and_get_kva_of(uintptr_t va, pcb_t *owner_pcb){
    
    uintptr_t kva = get_kva_of(va, owner_pcb->pgdir);
    if(!kva){
        // the page that `va` points to has been swapped out memory
        // therefore we should swap in first
        list_node_t *swap_node = find_and_pop_swp_node(va, owner_pcb);
        if(!swap_node){
            return 0;
        }
        // assert(swap_node);
        swap_in(LIST2SWP(swap_node), owner_pcb);
    }
    kva = get_kva_of(va, owner_pcb->pgdir);
    assert(kva);
    return kva;
}

// NOTE: Only need for S-core to alloc 2MB large page
#ifdef S_CORE
static ptr_t largePageMemCurr = LARGE_PAGE_FREEMEM;
ptr_t allocLargePage(int numPage)
{
    // align LARGE_PAGE_SIZE
    ptr_t ret = ROUND(largePageMemCurr, LARGE_PAGE_SIZE);
    largePageMemCurr = ret + numPage * LARGE_PAGE_SIZE;
    return ret;    
}
#endif

void freePage(ptr_t baseAddr)
{
    // TODO [P4-task1] (design you 'freePage' here if you need):
}

void clearPage(ptr_t kva){
    kva &= ~(NORMAL_PAGE_SIZE-1);
    uint64_t *base_addr = (uint64_t *)kva;
    for(int i=0;i<PAGE_SIZE/64;i++){
        base_addr[i] = 0;
    }
}

void *kmalloc(size_t size)
{
    // TODO [P4-task1] (design you 'kmalloc' here if you need):
    static uintptr_t free_head = 0;
    static uintptr_t free_tail = 0;
    
    // alignment
    // Q: Should we align here?
    // do it anyway
    while(size%16){
        size++;
    }

    if(free_head+size>free_tail){
        // drop the rest space
        // alloc new space to meet the size
        int needed_page = (size - 1 + PAGE_SIZE) / PAGE_SIZE;
        free_head = allocPage(needed_page);
        free_tail = free_head + needed_page;
    }

    uintptr_t ret = free_head;
    free_head += size;
    return (void *)ret;
}


/* this is used for mapping kernel virtual address into user page table */
void share_pgtable(uintptr_t dest_pgdir, uintptr_t src_pgdir)
{
    // TODO [P4-task1] share_pgtable:
    PTE *src = (PTE *)src_pgdir;
    PTE *dest = (PTE *)dest_pgdir;

    // kva = 0xffff_ffc0_5xxx_xxxx
    uintptr_t kva = pa2kva(PGDIR_PA);
    kva &= VA_MASK;
    uint64_t vpn2 = kva >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
    dest[vpn2] = src[vpn2];
    
    // for [p5-task1]
    // iova = 0xffff_ffe0_?xxx_xxxx // 1GB
    uintptr_t iova = IO_ADDR_START;
    iova &= VA_MASK;
    vpn2 = iova >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
    dest[vpn2] = src[vpn2];
}

/* allocate physical page for `va`, mapping it into `pgdir`,
   return the kernel virtual address for the page
   */
uintptr_t alloc_page_helper(uintptr_t va, /*uintptr_t pgdir*/pcb_t *owner_pcb)
{
    // TODO [P4-task1] alloc_page_helper:
    printl("[in alloc_page_helper]\n");

    va &= VA_MASK;
    uint64_t vpn2 = va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
    uint64_t vpn1 = (vpn2 << PPN_BITS) ^ (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
    uint64_t vpn0 = (vpn2 << (PPN_BITS + PPN_BITS)) ^ (vpn1 << PPN_BITS) ^ (va >> (NORMAL_PAGE_SHIFT));

    PTE *pgd = (PTE *)owner_pcb->pgdir;
    // if(!pgd[vpn2]){
    if(!get_attribute(pgd[vpn2], _PAGE_PRESENT)){
        // alloc a new page directory
        set_pfn(&pgd[vpn2], kva2pa(
            // allocPage(1)
            alloc_page_from_pool(0, owner_pcb)
        )>>NORMAL_PAGE_SHIFT);
        set_attribute(&pgd[vpn2], _PAGE_PRESENT | _PAGE_USER);
        clear_pgdir(pa2kva(get_pa(pgd[vpn2])));
    }

    PTE *pmd = (PTE *)pa2kva(get_pa(pgd[vpn2]));
    // if(!pmd[vpn1]){
    if(!get_attribute(pmd[vpn1], _PAGE_PRESENT)){
        // alloc a new page directory
        set_pfn(&pmd[vpn1], kva2pa(
            // allocPage(1)
            alloc_page_from_pool(0, owner_pcb)
        )>>NORMAL_PAGE_SHIFT);
        set_attribute(&pmd[vpn1], _PAGE_PRESENT | _PAGE_USER);
        clear_pgdir(pa2kva(get_pa(pmd[vpn1])));
    }

    PTE *pte = (PTE *)pa2kva(get_pa(pmd[vpn1]));
    // if(!pte[vpn0]){
    // if(!get_attribute(pte[vpn0], _PAGE_PRESENT)){
    // alloc a new page directory
    set_pfn(&pte[vpn0], kva2pa(
        // allocPage(1)
        alloc_page_from_pool(va, owner_pcb)
    )>>NORMAL_PAGE_SHIFT);
    set_attribute(&pte[vpn0], _PAGE_PRESENT | _PAGE_READ | _PAGE_WRITE | _PAGE_EXEC | 
                                _PAGE_USER /*| _PAGE_ACCESSED | _PAGE_DIRTY*/);
    // clear_pgdir(pa2kva(get_pa(pte[vpn0]))); // Must we clear here ?
    // }

    print_va_at_pgdir(va, owner_pcb->pgdir);
    local_flush_tlb_all();
    printl("[leave alloc_page_helper]\n");
    return pa2kva(get_pa(pte[vpn0]));
}

// alloc pgdir from pool
void map_page(uintptr_t va, uintptr_t pa, pcb_t *owner_pcb)
{
    printl("[in map_page]\n");

    va &= VA_MASK;
    uint64_t vpn2 = va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
    uint64_t vpn1 = (vpn2 << PPN_BITS) ^ (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
    uint64_t vpn0 = (vpn2 << (PPN_BITS + PPN_BITS)) ^ (vpn1 << PPN_BITS) ^ (va >> (NORMAL_PAGE_SHIFT));

    PTE *pgd = (PTE *)owner_pcb->pgdir;
    // if(!pgd[vpn2]){
    if(!get_attribute(pgd[vpn2], _PAGE_PRESENT)){
        // alloc a new page directory
        set_pfn(&pgd[vpn2], kva2pa(
            // allocPage(1)
            alloc_page_from_pool(0, owner_pcb)
        )>>NORMAL_PAGE_SHIFT);
        set_attribute(&pgd[vpn2], _PAGE_PRESENT | _PAGE_USER);
        clear_pgdir(pa2kva(get_pa(pgd[vpn2])));
    }

    PTE *pmd = (PTE *)pa2kva(get_pa(pgd[vpn2]));
    // if(!pmd[vpn1]){
    if(!get_attribute(pmd[vpn1], _PAGE_PRESENT)){
        // alloc a new page directory
        set_pfn(&pmd[vpn1], kva2pa(
            // allocPage(1)
            alloc_page_from_pool(0, owner_pcb)
        )>>NORMAL_PAGE_SHIFT);
        set_attribute(&pmd[vpn1], _PAGE_PRESENT | _PAGE_USER);
        clear_pgdir(pa2kva(get_pa(pmd[vpn1])));
    }

    PTE *pte = (PTE *)pa2kva(get_pa(pmd[vpn1]));
    assert(!get_attribute(pte[vpn0], _PAGE_PRESENT));
    set_pfn(&pte[vpn0], pa>>NORMAL_PAGE_SHIFT);
    set_attribute(&pte[vpn0], _PAGE_PRESENT | _PAGE_READ | _PAGE_WRITE | _PAGE_EXEC | 
                                _PAGE_USER /*| _PAGE_ACCESSED | _PAGE_DIRTY*/);
    
    local_flush_tlb_all();
    printl("[leave map_page]\n");
}

// for [p5-task1]
// alloc pgdir from kernMemCurr
void map_page_2(uintptr_t va, uintptr_t pa, PTE *pgdir)
{
    // printl("[in map_page_2]\n");

    va &= VA_MASK;
    uint64_t vpn2 = va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
    uint64_t vpn1 = (vpn2 << PPN_BITS) ^ (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
    uint64_t vpn0 = (vpn2 << (PPN_BITS + PPN_BITS)) ^ (vpn1 << PPN_BITS) ^ (va >> (NORMAL_PAGE_SHIFT));

    PTE *pgd = (PTE *)pgdir;
    // if(!pgd[vpn2]){
    if(!get_attribute(pgd[vpn2], _PAGE_PRESENT)){
        // alloc a new page directory
        set_pfn(&pgd[vpn2], kva2pa(
            allocPage(1)
            // alloc_page_from_pool(0, owner_pcb)
        )>>NORMAL_PAGE_SHIFT);
        set_attribute(&pgd[vpn2], _PAGE_PRESENT | _PAGE_USER);
        clear_pgdir(pa2kva(get_pa(pgd[vpn2])));
    }

    PTE *pmd = (PTE *)pa2kva(get_pa(pgd[vpn2]));
    // if(!pmd[vpn1]){
    if(!get_attribute(pmd[vpn1], _PAGE_PRESENT)){
        // alloc a new page directory
        set_pfn(&pmd[vpn1], kva2pa(
            allocPage(1)
            // alloc_page_from_pool(0, owner_pcb)
        )>>NORMAL_PAGE_SHIFT);
        set_attribute(&pmd[vpn1], _PAGE_PRESENT | _PAGE_USER);
        clear_pgdir(pa2kva(get_pa(pmd[vpn1])));
    }

    PTE *pte = (PTE *)pa2kva(get_pa(pmd[vpn1]));
    assert(!get_attribute(pte[vpn0], _PAGE_PRESENT));
    set_pfn(&pte[vpn0], pa>>NORMAL_PAGE_SHIFT);
    set_attribute(&pte[vpn0], _PAGE_PRESENT | _PAGE_READ | _PAGE_WRITE | _PAGE_EXEC | 
                                _PAGE_USER | _PAGE_ACCESSED | _PAGE_DIRTY);
    
    local_flush_tlb_all();
    // printl("[leave map_page_2]\n");
}

shm_t shms[NUM_MAX_SHMPAGE];

uintptr_t shm_page_get(int key)
{
    // TODO [P4-task4] shm_page_get:
    pcb_t *father_pcb = find_father_pcb_for_tcb(current_running_of[get_current_cpu_id()]);

    for(int i=0;i<NUM_MAX_SHMPAGE;i++){
        if(shms[i].handle_num>0 && shms[i].key==key){
            shms[i].handle_num++;

            // find an available va
            for(
                uintptr_t va = SHMPAGE_VA_BASE + (i<<NORMAL_PAGE_SHIFT);
                va < SHMPAGE_VA_BASE + SHMPAGE_VA_SIZE;
                va += NUM_MAX_SHMPAGE<<NORMAL_PAGE_SHIFT
            ){
                if(!check_and_get_kva_of(va, father_pcb)){
                    // not used
                    map_page(va, kva2pa(shms[i].pf->kva), father_pcb);
                    return va;
                }
            }

            // no available shm va
            assert(0);
        }
    }

    for(int i=0;i<NUM_MAX_SHMPAGE;i++){
        if(shms[i].handle_num==0){
            shms[i].handle_num++;
            shms[i].key = key;

            // alloc a new page frame from free page frame pool
            if(list_is_empty(&free_pf_pool)){
                swap_out_randomly();
            }
            shms[i].pf = LIST2PF(list_pop(&free_pf_pool));
            shms[i].pf->va = 0;
            shms[i].pf->owner = 0;

            // clear the page frame after creating
            clearPage(shms[i].pf->kva);

            // find an available va
            for(
                uintptr_t va = SHMPAGE_VA_BASE + (i<<NORMAL_PAGE_SHIFT);
                va < SHMPAGE_VA_BASE + SHMPAGE_VA_SIZE;
                va += NUM_MAX_SHMPAGE<<NORMAL_PAGE_SHIFT
            ){
                if(!check_and_get_kva_of(va, father_pcb)){
                    // not used
                    map_page(va, kva2pa(shms[i].pf->kva), father_pcb);
                    return va;
                }
            }

            // no available shm va
            assert(0);
        }
    }

    // no available shm table
    assert(0);
    return 0;
}

void shm_page_dt(uintptr_t addr)
{
    // TODO [P4-task4] shm_page_dt:
    pcb_t *father_pcb = find_father_pcb_for_tcb(current_running_of[get_current_cpu_id()]);

    uintptr_t kva = get_kva_of(addr, father_pcb->pgdir);
    for(int i=0;i<NUM_MAX_SHMPAGE;i++){
        if(shms[i].handle_num>0 && shms[i].pf->kva==kva){
            set_pte_invalid(addr, father_pcb->pgdir);
            if(!--shms[i].handle_num){
                // todo: also release occupied shm page when exiting or being killed
                list_push(&free_pf_pool, &shms[i].pf->list);
            }
            return;
        }
    }

    // no such addr
    assert(0);
}

// for [p4-task6]
// note: the page va points to might have been swapped out of memory
//       and the func will return 0 under such circumstance
// on found, reuturn pa; else return 0
uintptr_t get_pa_of(uintptr_t va){
    uintptr_t kva;
    if((kva=get_kva_of(va, current_running_of[get_current_cpu_id()]->pgdir))){
        return kva2pa(kva);
    }else{
        return 0;
    }
}

uintptr_t take_snapshot(uintptr_t pg_va){
    pcb_t *father_pcb = find_father_pcb_for_tcb(current_running_of[get_current_cpu_id()]);

    // find an available va
    uintptr_t va;
    for(
        va = SNAPSHOT_VA_BASE;
        va < SNAPSHOT_VA_BASE + SNAPSHOT_VA_SIZE;
        va += NORMAL_PAGE_SIZE
    ){
        if(!check_and_get_kva_of(va, father_pcb)){
            // not used
            map_page(va, kva2pa(check_and_get_kva_of(pg_va, father_pcb)), father_pcb);
            break;
        }
    }
    assert(va!=SNAPSHOT_VA_BASE+SNAPSHOT_VA_SIZE);

    // set read-exec only
    PTE *pg_pteptr = get_pte_of(pg_va, father_pcb->pgdir);
    assert(pg_pteptr);
    set_attribute(pg_pteptr, ~_PAGE_WRITE & get_attribute(*pg_pteptr, 0xfflu));

    return va;
}