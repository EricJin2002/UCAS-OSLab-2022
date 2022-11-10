#include <os/mm.h>
#include <assert.h>

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
    return ret;
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
}

/* allocate physical page for `va`, mapping it into `pgdir`,
   return the kernel virtual address for the page
   */
uintptr_t alloc_page_helper(uintptr_t va, uintptr_t pgdir)
{
    // TODO [P4-task1] alloc_page_helper:
    va &= VA_MASK;
    uint64_t vpn2 = va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
    uint64_t vpn1 = (vpn2 << PPN_BITS) ^ (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
    uint64_t vpn0 = (vpn2 << (PPN_BITS + PPN_BITS)) ^ (vpn1 << PPN_BITS) ^ (va >> (NORMAL_PAGE_SHIFT));

    PTE *pgd = (PTE *)pgdir;
    // if(!pgd[vpn2]){
    if(!get_attribute(pgd[vpn2], _PAGE_PRESENT)){
        // alloc a new page directory
        set_pfn(&pgd[vpn2], kva2pa(allocPage(1))>>NORMAL_PAGE_SHIFT);
        set_attribute(&pgd[vpn2], _PAGE_PRESENT | _PAGE_USER);
        clear_pgdir(pa2kva(get_pa(pgd[vpn2])));
    }

    PTE *pmd = (PTE *)pa2kva(get_pa(pgd[vpn2]));
    // if(!pmd[vpn1]){
    if(!get_attribute(pmd[vpn1], _PAGE_PRESENT)){
        // alloc a new page directory
        set_pfn(&pmd[vpn1], kva2pa(allocPage(1))>>NORMAL_PAGE_SHIFT);
        set_attribute(&pmd[vpn1], _PAGE_PRESENT | _PAGE_USER);
        clear_pgdir(pa2kva(get_pa(pmd[vpn1])));
    }

    PTE *pte = (PTE *)pa2kva(get_pa(pmd[vpn1]));
    // if(!pte[vpn0]){
    if(!get_attribute(pte[vpn0], _PAGE_PRESENT)){
        // alloc a new page directory
        set_pfn(&pte[vpn0], kva2pa(allocPage(1))>>NORMAL_PAGE_SHIFT);
        set_attribute(&pte[vpn0], _PAGE_PRESENT | _PAGE_READ | _PAGE_WRITE | _PAGE_EXEC | 
                                    _PAGE_USER | _PAGE_ACCESSED | _PAGE_DIRTY);
        // clear_pgdir(pa2kva(get_pa(pte[vpn0]))); // Must we clear here ?
    }

    printl("[in alloc_page_helper]\n");
    print_va_at_pgdir(va, pgdir);
    printl("\n");
    return pa2kva(get_pa(pte[vpn0]));
}

uintptr_t shm_page_get(int key)
{
    // TODO [P4-task4] shm_page_get:
}

void shm_page_dt(uintptr_t addr)
{
    // TODO [P4-task4] shm_page_dt:
}