#include <stdio.h>
#include <unistd.h>

static int data[1200];

void add1(){
    // sys_move_cursor(0, 7);
    long sum;
    for(int i=0;i<600;i++){
        sys_move_cursor(0, 7);
        printf("add1 thread is calculating *ans1+=data[%d]", i);
        sum+=data[i];
    }
    sys_move_cursor(0, 7);
    printf("add1 thread finishes calculating *ans1=%d", sum);
    sys_thread_exit((void *)(long)sum);
    // while(1);
}

void add2(){
    // sys_move_cursor(0, 8);
    long sum;
    for(int i=600;i<1200;i++){
        sys_move_cursor(0, 8);
        printf("add2 thread is calculating *ans2+=data[%d]", i);
        sum+=data[i];
    }
    sys_move_cursor(0, 8);
    printf("add2 thread finishes calculating *ans2=%d", sum);
    sys_thread_exit((void *)(long)sum);
    // while(1);
}

int main(void)
{
    int print_location = 6;

    for(int i=0;i<1200;i++){
        data[i]=i;
    }

    int32_t tid1,tid2;

    sys_thread_create(&tid1, (uint64_t)add1, 0L, 0L, 0L);
    sys_thread_create(&tid2, (uint64_t)add2, 0L, 0L, 0L);

    sys_move_cursor(0, print_location);
    printf("> [TASK] Thread created: tid1=%d tid2=%d", tid1, tid2);

    // while(!finished1||!finished2);
    long ans_from_join1, ans_from_join2;
    sys_thread_join(tid1, (void **)&ans_from_join1);
    sys_move_cursor(0, 7);
    printf("> [TASK] Thread joined: tid1=%d ans_from_join1=%d", tid1, ans_from_join1);
    sys_thread_join(tid2, (void **)&ans_from_join2);
    sys_move_cursor(0, 8);
    printf("> [TASK] Thread joined: tid2=%d ans_from_join2=%d", tid2, ans_from_join2);

    sys_move_cursor(0, print_location);
    printf("> [TASK] This task is to test thread_create. (%d)", (long)ans_from_join1+(long)ans_from_join2);
    while(1);
}
