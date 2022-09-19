#include <stdio.h>
// #include <unistd.h>  // NOTE: use this header after implementing syscall!
#include <kernel.h>

/**
 * The ascii airplane is designed by Joan Stark
 * from: https://www.asciiart.eu/vehicles/airplanes
 */

static char blank[] = {"                                                                   "};
static char plane1[] = {"         _       "};
static char plane2[] = {"       -=\\`\\     "};
static char plane3[] = {"   |\\ ____\\_\\__  "};
static char plane4[] = {" -=\\c`\"\"\"\"\"\"\" \"`)"};
static char plane5[] = {"    `~~~~~/ /~~` "};
static char plane6[] = {"      -==/ /     "};
static char plane7[] = {"        '-'      "};

/**
 * NOTE: kernel APIs is used for p2-task1 and p2-task2. You need to change
 * to syscall APIs after implementing syscall in p2-task3!
*/
int main(void)
{
    int j = 10;

    while (1)
    {
        for (int i = 0; i < 50; i++)
        {
            /* move */
            kernel_move_cursor(i, j + 0);
            kernel_print("%s", (long)plane1, 0);

            kernel_move_cursor(i, j + 1);
            kernel_print("%s", (long)plane2, 0);

            kernel_move_cursor(i, j + 2);
            kernel_print("%s", (long)plane3, 0);

            kernel_move_cursor(i, j + 3);
            kernel_print("%s", (long)plane4, 0);

            kernel_move_cursor(i, j + 4);
            kernel_print("%s", (long)plane5, 0);

            kernel_move_cursor(i, j + 5);
            kernel_print("%s", (long)plane6, 0);

            kernel_move_cursor(i, j + 6);
            kernel_print("%s", (long)plane7, 0);
        }
        kernel_yield();

        kernel_move_cursor(0, j + 0);
        kernel_print("%s", (long)blank, 0);

        kernel_move_cursor(0, j + 1);
        kernel_print("%s", (long)blank, 0);

        kernel_move_cursor(0, j + 2);
        kernel_print("%s", (long)blank, 0);

        kernel_move_cursor(0, j + 3);
        kernel_print("%s", (long)blank, 0);

        kernel_move_cursor(0, j + 4);
        kernel_print("%s", (long)blank, 0);

        kernel_move_cursor(0, j + 5);
        kernel_print("%s", (long)blank, 0);

        kernel_move_cursor(0, j + 6);
        kernel_print("%s", (long)blank, 0);
    }
}

// int main(void)
// {
//     int j = 10;

//     while (1)
//     {
//         for (int i = 0; i < 50; i++)
//         {
//             /* move */
//             sys_move_cursor(i, j + 0);
//             printf("%s", plane1);

//             sys_move_cursor(i, j + 1);
//             printf("%s", plane2);

//             sys_move_cursor(i, j + 2);
//             printf("%s", plane3);

//             sys_move_cursor(i, j + 3);
//             printf("%s", plane4);

//             sys_move_cursor(i, j + 4);
//             printf("%s", plane5);

//             sys_move_cursor(i, j + 5);
//             printf("%s", plane6);

//             sys_move_cursor(i, j + 6);
//             printf("%s", plane7);
//         }
//         sys_yield();

//         sys_move_cursor(0, j + 0);
//         printf("%s", blank);

//         sys_move_cursor(0, j + 1);
//         printf("%s", blank);

//         sys_move_cursor(0, j + 2);
//         printf("%s", blank);

//         sys_move_cursor(0, j + 3);
//         printf("%s", blank);

//         sys_move_cursor(0, j + 4);
//         printf("%s", blank);

//         sys_move_cursor(0, j + 5);
//         printf("%s", blank);

//         sys_move_cursor(0, j + 6);
//         printf("%s", blank);
//     }
// }
