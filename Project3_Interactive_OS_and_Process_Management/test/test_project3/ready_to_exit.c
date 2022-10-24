#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>

int main(int argc, char *argv[])
{
    assert(argc >= 4);
    int print_location = atoi(argv[1]);
    int handle1 = atoi(argv[2]);
    int handle2 = atoi(argv[3]);
    // Acquire two mutex locks
    sys_mutex_acquire(handle1);
    sys_mutex_acquire(handle2);

    // Start for-loop, wait for timeup or being killed
    for (int i = 0; i < 10000; ++i)
    {
        sys_move_cursor(0, print_location);
        printf("> [TASK] I am task with pid %d, I have acquired two mutex locks. (%d)", sys_getpid(), i);
    }

    // If timeup, release two mutex locks
    sys_mutex_release(handle1);
    sys_mutex_release(handle2);

    return 0;
}