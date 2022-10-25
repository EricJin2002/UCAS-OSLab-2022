#include <asm.h>

.section ".entry_function","ax"
ENTRY(_start)

    /* TODO: [p1-task3] setup C runtime environment for the user program */

    // clear bss
    la t0, __bss_start
    la t1, __BSS_END__
    bge t0, t1, clear_bss_done
clear_bss:
    sw zero, 0(t0)
    addi t0, t0, 4
    blt t0, t1, clear_bss
clear_bss_done:

    //save ra
    // addi sp, sp, -8
    // sd ra, 0(sp)

    /* TODO: [p1-task3] enter main function */
    call main

    /* TODO: [p1-task3] finish task and return to the kernel */
    /* NOTE: You need to replace this with new mechanism in p3-task2! */
    // ld ra, 0(sp)
    // addi sp, sp, 8
    // ret

    /* TODO: [P3] call sys_exit */
    call sys_exit
    
// while(1) loop, unreachable here
loop:
    wfi
    j loop

END(_start)
