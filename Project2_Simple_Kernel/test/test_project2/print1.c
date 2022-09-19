#include <stdio.h>
// #include <unistd.h>  // NOTE: use this header after implementing syscall!
#include <kernel.h>

/**
 * NOTE: kernel APIs is used for p2-task1 and p2-task2. You need to change
 * to syscall APIs after implementing syscall in p2-task3!
*/
int main(void)
{
    int print_location = 0;

    for (int i = 0;; i++)
    {
        kernel_move_cursor(0, print_location);
        kernel_print("> [TASK] This task is to test scheduler. (%d)", i, 0);
        kernel_yield();
    }
}

// int main(void)
// {
//     int print_location = 0;

//     for (int i = 0;; i++)
//     {
//         sys_move_cursor(0, print_location);
//         printf("> [TASK] This task is to test scheduler. (%d)", i);
//         sys_yield();
//     }
// }
