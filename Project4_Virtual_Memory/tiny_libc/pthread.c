#include <pthread.h>

/* TODO:[P4-task4] pthread_create/wait */
void pthread_create(pthread_t *thread,
                   void (*start_routine)(void*),
                   void *arg)
{
    /* TODO: [p4-task4] implement pthread_create */
    sys_thread_create((int32_t *)thread, (uint64_t)start_routine, (long)arg, 0, 0);
}

int pthread_join(pthread_t thread)
{
    /* TODO: [p4-task4] implement pthread_join */
    long retval;
    sys_thread_join((int32_t)thread, (void **)&retval);
    return (int)retval; //Q: what should I return here ?
}
