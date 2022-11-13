#include <pthread.h>

/* TODO:[P4-task4] pthread_create/wait */
void pthread_create(pthread_t *thread,
                   void (*start_routine)(void*),
                   void *arg)
{
    /* TODO: [p4-task4] implement pthread_create */
    sys_thread_create(thread, start_routine, arg, 0, 0);
}

int pthread_join(pthread_t thread)
{
    /* TODO: [p4-task4] implement pthread_join */
    int retval;
    sys_thread_join(thread, &retval);
    return retval; //Q: what should I return here ?
}
