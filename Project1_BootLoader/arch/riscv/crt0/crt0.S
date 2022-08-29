#include <asm.h>

ENTRY(_start)

    /* TODO: [p1-task3] setup C runtime environment for the user program */


    /* TODO: [p1-task3] enter main function */


    /* TODO: [p1-task3] finish task and return to the kernel */
    /* NOTE: You need to replace this with new mechanism in p3-task2! */


// while(1) loop, unreachable here
loop:
    wfi
    j loop

END(_start)
