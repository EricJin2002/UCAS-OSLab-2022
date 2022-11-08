#include <syscall.h>
#include <stdint.h>
#include <unistd.h>

static const long IGNORE = 0L;

static long invoke_syscall(long sysno, long arg0, long arg1, long arg2,
                           long arg3, long arg4)
{
    /* TODO: [p2-task3] implement invoke_syscall via inline assembly */    

// abandoned due to O2 failed !
//    long ret;
//    asm volatile(
//        "mv a7, a0\n"
//        "mv a0, a1\n"
//        "mv a1, a2\n"
//        "mv a2, a3\n"
//        "mv a3, a4\n"
//        "mv a4, a5\n"
//        "ecall\n"
//        "mv %0, a0\n"
//        :"=r"(ret)
//    );

    long ret;
    asm volatile(
        "mv a7, %1\n"
        "mv a0, %2\n"
        "mv a1, %3\n"
        "mv a2, %4\n"
        "mv a3, %5\n"
        "mv a4, %6\n"
        "ecall\n"
        "mv %0, a0\n"
        :"=r"(ret)
        :"r"(sysno), "r"(arg0), "r"(arg1), "r"(arg2), "r"(arg3), "r"(arg4)
        :"a0","a1","a2","a3","a4","a7"
    );

    return ret;
}

void sys_yield(void)
{
    /* TODO: [p2-task3] call invoke_syscall to implement sys_yield */
    invoke_syscall(SYSCALL_YIELD, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_move_cursor(int x, int y)
{
    /* TODO: [p2-task3] call invoke_syscall to implement sys_move_cursor */
    invoke_syscall(SYSCALL_CURSOR, (long)x, (long)y, IGNORE, IGNORE, IGNORE);
}

void sys_write(char *buff)
{
    /* TODO: [p2-task3] call invoke_syscall to implement sys_write */
    invoke_syscall(SYSCALL_WRITE, (long)buff, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_reflush(void)
{
    /* TODO: [p2-task3] call invoke_syscall to implement sys_reflush */
    invoke_syscall(SYSCALL_REFLUSH, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}

// for [p3-task1]
void sys_clear(void){
    invoke_syscall(SYSCALL_CLEAR, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}
void sys_backspace(void){
    invoke_syscall(SYSCALL_BACKSPACE, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}


int sys_mutex_init(int key)
{
    /* TODO: [p2-task3] call invoke_syscall to implement sys_mutex_init */
    return invoke_syscall(SYSCALL_LOCK_INIT, (long)key, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_mutex_acquire(int mutex_idx)
{
    /* TODO: [p2-task3] call invoke_syscall to implement sys_mutex_acquire */
    invoke_syscall(SYSCALL_LOCK_ACQ, (long)mutex_idx, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_mutex_release(int mutex_idx)
{
    /* TODO: [p2-task3] call invoke_syscall to implement sys_mutex_release */
    invoke_syscall(SYSCALL_LOCK_RELEASE, (long)mutex_idx, IGNORE, IGNORE, IGNORE, IGNORE);
}

long sys_get_timebase(void)
{
    /* TODO: [p2-task3] call invoke_syscall to implement sys_get_timebase */
    return invoke_syscall(SYSCALL_GET_TIMEBASE, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}

long sys_get_tick(void)
{
    /* TODO: [p2-task3] call invoke_syscall to implement sys_get_tick */
    return invoke_syscall(SYSCALL_GET_TICK, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_sleep(uint32_t time)
{
    /* TODO: [p2-task3] call invoke_syscall to implement sys_sleep */
    invoke_syscall(SYSCALL_SLEEP, (long)time, IGNORE, IGNORE, IGNORE, IGNORE);
}

// for [p2-task5]
void sys_thread_create(int32_t *tidptr, uint64_t entrypoint, long a0, long a1, long a2){
    invoke_syscall(SYSCALL_THREAD_CREATE, (long)tidptr, entrypoint, a0, a1, a2);
}
void sys_thread_exit(void *retval){
    invoke_syscall(SYSCALL_THREAD_EXIT, (long)retval, IGNORE, IGNORE, IGNORE, IGNORE);
}
int sys_thread_join(int32_t tid, void **retvalptr){
    return invoke_syscall(SYSCALL_THREAD_JOIN, (long)tid, (long)retvalptr, IGNORE, IGNORE, IGNORE);
}

#ifdef S_CORE
// S-core
pid_t  sys_exec(int id, int argc, uint64_t arg0, uint64_t arg1, uint64_t arg2)
{
    /* TODO: [p3-task1] call invoke_syscall to implement sys_exec for S_CORE */
    return invoke_syscall(SYSCALL_EXEC, (long)id, (long)argc, (long)arg0, (long)arg1, (long)arg2);
}
#else
// A/C-core
pid_t  sys_exec(char *name, int argc, char **argv)
{
    /* TODO: [p3-task1] call invoke_syscall to implement sys_exec */
    return invoke_syscall(SYSCALL_EXEC, (long)name, (long)argc, (long)argv, IGNORE, IGNORE);
}
#endif


void sys_exit(void)
{
    /* TODO: [p3-task1] call invoke_syscall to implement sys_exit */
    invoke_syscall(SYSCALL_EXIT, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}

int  sys_kill(pid_t pid)
{
    /* TODO: [p3-task1] call invoke_syscall to implement sys_kill */
    return invoke_syscall(SYSCALL_KILL, (long)pid, IGNORE, IGNORE, IGNORE, IGNORE);
}

int  sys_waitpid(pid_t pid)
{
    /* TODO: [p3-task1] call invoke_syscall to implement sys_waitpid */
    return invoke_syscall(SYSCALL_WAITPID, (long)pid, IGNORE, IGNORE, IGNORE, IGNORE);
}


void sys_ps(void)
{
    /* TODO: [p3-task1] call invoke_syscall to implement sys_ps */
    invoke_syscall(SYSCALL_PS, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}

pid_t sys_getpid()
{
    /* TODO: [p3-task1] call invoke_syscall to implement sys_getpid */
    return invoke_syscall(SYSCALL_GETPID, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}

int  sys_getchar(void)
{
    /* TODO: [p3-task1] call invoke_syscall to implement sys_getchar */
    return invoke_syscall(SYSCALL_READCH, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}

// for [p3-task1]
void sys_show_task(void){
    invoke_syscall(SYSCALL_SHOW_TASK, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}

int  sys_barrier_init(int key, int goal)
{
    /* TODO: [p3-task2] call invoke_syscall to implement sys_barrier_init */
    return invoke_syscall(SYSCALL_BARR_INIT, (long)key, (long)goal, IGNORE, IGNORE, IGNORE);
}

void sys_barrier_wait(int bar_idx)
{
    /* TODO: [p3-task2] call invoke_syscall to implement sys_barrie_wait */
    invoke_syscall(SYSCALL_BARR_WAIT, (long)bar_idx, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_barrier_destroy(int bar_idx)
{
    /* TODO: [p3-task2] call invoke_syscall to implement sys_barrie_destory */
    invoke_syscall(SYSCALL_BARR_DESTROY, (long)bar_idx, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_condition_init(int key)
{
    /* TODO: [p3-task2] call invoke_syscall to implement sys_condition_init */
    return invoke_syscall(SYSCALL_COND_INIT, (long)key, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_condition_wait(int cond_idx, int mutex_idx)
{
    /* TODO: [p3-task2] call invoke_syscall to implement sys_condition_wait */
    invoke_syscall(SYSCALL_COND_WAIT, (long)cond_idx, (long)mutex_idx, IGNORE, IGNORE, IGNORE);
}

void sys_condition_signal(int cond_idx)
{
    /* TODO: [p3-task2] call invoke_syscall to implement sys_condition_signal */
    invoke_syscall(SYSCALL_COND_SIGNAL, (long)cond_idx, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_condition_broadcast(int cond_idx)
{
    /* TODO: [p3-task2] call invoke_syscall to implement sys_condition_broadcast */
    invoke_syscall(SYSCALL_COND_BROADCAST, (long)cond_idx, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_condition_destroy(int cond_idx)
{
    /* TODO: [p3-task2] call invoke_syscall to implement sys_condition_destroy */
    invoke_syscall(SYSCALL_COND_DESTROY, (long)cond_idx, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_mbox_open(char * name)
{
    /* TODO: [p3-task2] call invoke_syscall to implement sys_mbox_open */
    return invoke_syscall(SYSCALL_MBOX_OPEN, (long)name, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_mbox_close(int mbox_id)
{
    /* TODO: [p3-task2] call invoke_syscall to implement sys_mbox_close */
    invoke_syscall(SYSCALL_MBOX_CLOSE, (long)mbox_id, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_mbox_send(int mbox_idx, void *msg, int msg_length)
{
    /* TODO: [p3-task2] call invoke_syscall to implement sys_mbox_send */
    return invoke_syscall(SYSCALL_MBOX_SEND, (long)mbox_idx, (long)msg, (long)msg_length, IGNORE, IGNORE);
}

int sys_mbox_recv(int mbox_idx, void *msg, int msg_length)
{
    /* TODO: [p3-task2] call invoke_syscall to implement sys_mbox_recv */
    return invoke_syscall(SYSCALL_MBOX_RECV, (long)mbox_idx, (long)msg, (long)msg_length, IGNORE, IGNORE);
}

// for [p3-task4]
int sys_taskset_name(int mask, char *name, int argc, char **argv){
    return invoke_syscall(SYSCALL_TASKSET_NAME, (long)mask, (long)name, (long)argc, (long)argv, IGNORE);
}

int sys_taskset_pid(int mask, pid_t pid){
    return invoke_syscall(SYSCALL_TASKSET_PID, (long)mask, (long)pid, IGNORE, IGNORE, IGNORE);
}