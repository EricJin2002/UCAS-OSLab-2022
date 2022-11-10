#include <atomic.h>
#include <os/sched.h>
#include <os/smp.h>
#include <os/lock.h>
#include <os/kernel.h>

spin_lock_t kernel_lock;

void smp_init()
{
    /* TODO: P3-TASK3 multicore*/
    spin_lock_init(&kernel_lock);
}

void wakeup_other_hart()
{
    /* TODO: P3-TASK3 multicore*/
    send_ipi(0);
}

void lock_kernel()
{
    /* TODO: P3-TASK3 multicore*/
    // printl("core%d tries to acquire lock\n", get_current_cpu_id());
    spin_lock_acquire(&kernel_lock);
    // printl("core%d has acquired lock\n", get_current_cpu_id());
}

void unlock_kernel()
{
    /* TODO: P3-TASK3 multicore*/
    // printl("core%d has released lock\n", get_current_cpu_id());
    spin_lock_release(&kernel_lock);
}
