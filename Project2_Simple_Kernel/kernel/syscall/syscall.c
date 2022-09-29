#include <sys/syscall.h>

long (*syscall[NUM_SYSCALLS])();

void handle_syscall(regs_context_t *regs, uint64_t interrupt, uint64_t cause)
{
    /* TODO: [p2-task3] handle syscall exception */
    /**
     * HINT: call syscall function like syscall[fn](arg0, arg1, arg2),
     * and pay attention to the return value and sepc
     */
    //printl("enter handle_syscall\n");
    //a0 = syscall[a7](a0, a1, a2, a3, a4);
    printl("syscall \ta7 %d \ta0 %d \ta1 %d \ta2 %d \t",regs->regs[17],regs->regs[10],regs->regs[11],regs->regs[12]);
    regs->regs[10] = syscall[regs->regs[17]](
        regs->regs[10], 
        regs->regs[11], 
        regs->regs[12], 
        regs->regs[13], 
        regs->regs[14]
    );
    printl("ret a0 %d\n",regs->regs[10]);
    //printl("leave handle_syscall with regs[a0] %lx\n", regs->regs[10]);
}
