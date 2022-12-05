#include <os/ioremap.h>
#include <os/mm.h>
#include <pgtable.h>
#include <type.h>

// maybe you can map it to IO_ADDR_START ?
static uintptr_t io_base = IO_ADDR_START;

void *ioremap(unsigned long phys_addr, unsigned long size)
{
    // TODO: [p5-task1] map one specific physical region to virtual address
    int num_of_pages = (size+PAGE_SIZE-1)/PAGE_SIZE;
    uintptr_t ret = io_base;
    while(num_of_pages--){
        map_page_2(io_base, phys_addr, pa2kva(PGDIR_PA));

        phys_addr+=PAGE_SIZE;
        io_base+=PAGE_SIZE;
    }
    print_va_at_pgdir(0xffffffe004003818, pa2kva(PGDIR_PA));
    return ret;
}

void iounmap(void *io_addr)
{
    // TODO: [p5-task1] a very naive iounmap() is OK
    // maybe no one would call this function?
}
