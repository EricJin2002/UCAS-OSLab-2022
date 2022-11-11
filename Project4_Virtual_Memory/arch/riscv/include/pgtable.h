#ifndef PGTABLE_H
#define PGTABLE_H

#include <type.h>
#include <printk.h>
#include <assert.h>

#define SATP_MODE_SV39 8
#define SATP_MODE_SV48 9

#define SATP_ASID_SHIFT 44lu
#define SATP_MODE_SHIFT 60lu

#define NORMAL_PAGE_SHIFT 12lu
#define NORMAL_PAGE_SIZE (1lu << NORMAL_PAGE_SHIFT)
#define LARGE_PAGE_SHIFT 21lu
#define LARGE_PAGE_SIZE (1lu << LARGE_PAGE_SHIFT)

/*
 * Flush entire local TLB.  'sfence.vma' implicitly fences with the instruction
 * cache as well, so a 'fence.i' is not necessary.
 */
static inline void local_flush_tlb_all(void)
{
    __asm__ __volatile__ ("sfence.vma" : : : "memory");
}

/* Flush one page from local TLB */
static inline void local_flush_tlb_page(unsigned long addr)
{
    __asm__ __volatile__ ("sfence.vma %0" : : "r" (addr) : "memory");
}

static inline void local_flush_icache_all(void)
{
    asm volatile ("fence.i" ::: "memory");
}

static inline void set_satp(
    unsigned mode, unsigned asid, unsigned long ppn)
{
    unsigned long __v =
        (unsigned long)(((unsigned long)mode << SATP_MODE_SHIFT) | ((unsigned long)asid << SATP_ASID_SHIFT) | ppn);
    __asm__ __volatile__("sfence.vma\ncsrw satp, %0" : : "rK"(__v) : "memory");
}

#define PGDIR_PA 0x51000000lu  // use 51000000 page as PGDIR

/*
 * PTE format:
 * | XLEN-1  10 | 9             8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0
 *       PFN      reserved for SW   D   A   G   U   X   W   R   V
 */

#define _PAGE_ACCESSED_OFFSET 6

#define _PAGE_PRESENT (1 << 0)
#define _PAGE_READ (1 << 1)     /* Readable */
#define _PAGE_WRITE (1 << 2)    /* Writable */
#define _PAGE_EXEC (1 << 3)     /* Executable */
#define _PAGE_USER (1 << 4)     /* User */
#define _PAGE_GLOBAL (1 << 5)   /* Global */
#define _PAGE_ACCESSED (1 << 6) /* Set by hardware on any access \
                                 */
#define _PAGE_DIRTY (1 << 7)    /* Set by hardware on any write */
#define _PAGE_SOFT (1 << 8)     /* Reserved for software */

#define _PAGE_PFN_SHIFT 10lu

#define VA_MASK ((1lu << 39) - 1)

#define PPN_BITS 9lu
#define NUM_PTE_ENTRY (1 << PPN_BITS)

typedef uint64_t PTE;

/* Translation between physical addr and kernel virtual addr */
static inline uintptr_t kva2pa(uintptr_t kva)
{
    /* TODO: [P4-task1] */
    return kva - 0xffffffc000000000lu;
}

static inline uintptr_t pa2kva(uintptr_t pa)
{
    /* TODO: [P4-task1] */
    return pa + 0xffffffc000000000lu;
}

/* get physical page addr from PTE 'entry' */
static inline uint64_t get_pa(PTE entry)
{
    /* TODO: [P4-task1] */
    return entry>>_PAGE_PFN_SHIFT<<NORMAL_PAGE_SHIFT;
}

/* Get/Set page frame number of the `entry` */
static inline long get_pfn(PTE entry)
{
    /* TODO: [P4-task1] */
    return entry>>_PAGE_PFN_SHIFT;
}
static inline void set_pfn(PTE *entry, uint64_t pfn)
{
    /* TODO: [P4-task1] */
    *entry = (*entry & ((1lu<<_PAGE_PFN_SHIFT)-1)) | (pfn<<_PAGE_PFN_SHIFT);
}

/* Get/Set attribute(s) of the `entry` */
static inline long get_attribute(PTE entry, uint64_t mask)
{
    /* TODO: [P4-task1] */
    return entry & mask;
}
static inline void set_attribute(PTE *entry, uint64_t bits)
{
    /* TODO: [P4-task1] */
    *entry = (*entry & ~0xfflu) | bits;
}

static inline void clear_pgdir(uintptr_t pgdir_addr)
{
    /* TODO: [P4-task1] */
    for(int i=0;i<NORMAL_PAGE_SIZE;i++){
        ((char *)pgdir_addr)[i]=0;
    }
}

/* 
 * query the page table stored in pgdir_va to obtain the physical 
 * address corresponding to the virtual address va.
 * 
 * return the kernel virtual address of the physical address 
 */
static inline uintptr_t get_kva_of(uintptr_t va, uintptr_t pgdir_va)
{
    // TODO: [P4-task1]
    va &= VA_MASK;
    uint64_t vpn2 = va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
    uint64_t vpn1 = (vpn2 << PPN_BITS) ^ (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
    uint64_t vpn0 = (vpn2 << (PPN_BITS + PPN_BITS)) ^ (vpn1 << PPN_BITS) ^ (va >> (NORMAL_PAGE_SHIFT));

    PTE *pgd = (PTE *)pgdir_va;
    if(!get_attribute(pgd[vpn2], _PAGE_PRESENT)){
        // no such entry
        return 0;
    }else if(get_attribute(pgd[vpn2], _PAGE_READ | _PAGE_WRITE | _PAGE_EXEC)){
        // pa found
        return pa2kva(get_pa(pgd[vpn2]) + (va & ((1lu << NORMAL_PAGE_SHIFT) - 1)));
    }

    PTE *pmd = (PTE *)pa2kva(get_pa(pgd[vpn2]));
    if(!get_attribute(pmd[vpn1], _PAGE_PRESENT)){
        // no such entry
        return 0;
    }else if(get_attribute(pmd[vpn1], _PAGE_READ | _PAGE_WRITE | _PAGE_EXEC)){
        // pa found
        return pa2kva(get_pa(pmd[vpn1]) + (va & ((1lu << NORMAL_PAGE_SHIFT) - 1)));
    }

    PTE *pte = (PTE *)pa2kva(get_pa(pmd[vpn1]));
    if(!get_attribute(pte[vpn0], _PAGE_PRESENT)){
        // no such entry
        return 0;
    }else if(get_attribute(pte[vpn0], _PAGE_READ | _PAGE_WRITE | _PAGE_EXEC)){
        // pa found
        return pa2kva(get_pa(pte[vpn0]) + (va & ((1lu << NORMAL_PAGE_SHIFT) - 1)));
    }

    return 0;
}

static inline void print_va_at_pgdir(uintptr_t va, uintptr_t pgdir_va){
    va &= VA_MASK;
    uint64_t vpn2 = va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
    uint64_t vpn1 = (vpn2 << PPN_BITS) ^ (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
    uint64_t vpn0 = (vpn2 << (PPN_BITS + PPN_BITS)) ^ (vpn1 << PPN_BITS) ^ (va >> (NORMAL_PAGE_SHIFT));

    printl("va    %lx\n", va);
    printl("vpn2  %lx\n", vpn2);
    printl("vpn1  %lx\n", vpn1);
    printl("vpn0  %lx\n", vpn0);

    PTE *pgd = (PTE *)pgdir_va;
    if(!get_attribute(pgd[vpn2], _PAGE_PRESENT)){
        // no such entry
        printl("pgd[vpn2] invalid!\n");
        return;
    }else if(get_attribute(pgd[vpn2], _PAGE_READ | _PAGE_WRITE | _PAGE_EXEC)){
        // pa found
        printl("pgd[vpn2] found pa %lx!\n",
            get_pa(pgd[vpn2]) + (va & ((1lu << NORMAL_PAGE_SHIFT) - 1)));
        return;
    }

    PTE *pmd = (PTE *)pa2kva(get_pa(pgd[vpn2]));
    if(!get_attribute(pmd[vpn1], _PAGE_PRESENT)){
        // no such entry
        printl("pmd[vpn1] invalid!\n");
        return;
    }else if(get_attribute(pmd[vpn1], _PAGE_READ | _PAGE_WRITE | _PAGE_EXEC)){
        // pa found
        printl("pmd[vpn1] found pa %lx!\n",
            get_pa(pmd[vpn1]) + (va & ((1lu << NORMAL_PAGE_SHIFT) - 1)));
        return;
    }

    PTE *pte = (PTE *)pa2kva(get_pa(pmd[vpn1]));
    if(!get_attribute(pte[vpn0], _PAGE_PRESENT)){
        // no such entry
        printl("pte[vpn0] invalid!\n");
        return;
    }else if(get_attribute(pte[vpn0], _PAGE_READ | _PAGE_WRITE | _PAGE_EXEC)){
        // pa found
        printl("pte[vpn0] found pa %lx!\n",
            get_pa(pte[vpn0]) + (va & ((1lu << NORMAL_PAGE_SHIFT) - 1)));
        return;
    }

    return;
}

static inline void set_pte_invalid(uintptr_t va, uintptr_t pgdir_va){
    va &= VA_MASK;
    uint64_t vpn2 = va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
    uint64_t vpn1 = (vpn2 << PPN_BITS) ^ (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
    uint64_t vpn0 = (vpn2 << (PPN_BITS + PPN_BITS)) ^ (vpn1 << PPN_BITS) ^ (va >> (NORMAL_PAGE_SHIFT));

    PTE *pgd = (PTE *)pgdir_va;
    assert(get_attribute(pgd[vpn2], _PAGE_PRESENT));
    assert(!get_attribute(pgd[vpn2], _PAGE_READ | _PAGE_WRITE | _PAGE_EXEC));

    PTE *pmd = (PTE *)pa2kva(get_pa(pgd[vpn2]));
    assert(get_attribute(pmd[vpn1], _PAGE_PRESENT));
    assert(!get_attribute(pmd[vpn1], _PAGE_READ | _PAGE_WRITE | _PAGE_EXEC));
    
    PTE *pte = (PTE *)pa2kva(get_pa(pmd[vpn1]));
    assert(get_attribute(pte[vpn0], _PAGE_PRESENT));
    assert(get_attribute(pte[vpn0], _PAGE_READ | _PAGE_WRITE | _PAGE_EXEC));
    
    set_attribute(&pte[vpn0], 0);
    local_flush_tlb_all();
    return;
}

#endif  // PGTABLE_H
