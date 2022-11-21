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

    // Try to acquire and release mutex lock1
    sys_move_cursor(0, print_location);
    printf("> [TASK] I want to acquire mutex lock1 (handle=%d).", handle1);

    sys_mutex_acquire(handle1);

    sys_move_cursor(0, print_location);
    printf("> [TASK] I have acquired mutex lock1 (handle=%d).  ", handle1);

    sys_mutex_release(handle1);

    // Try to acquire and release mutex lock2
    sys_move_cursor(0, print_location);
    printf("> [TASK] I want to acquire mutex lock2 (handle=%d).", handle2);

    sys_mutex_acquire(handle2);

    sys_move_cursor(0, print_location);
    printf("> [TASK] I have acquired mutex lock2 (handle=%d).  ", handle2);

    sys_mutex_release(handle2);

    return 0;
}