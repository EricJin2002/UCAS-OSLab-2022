#include <stdio.h>
#include <unistd.h>

static int data[1200];

void add1(int *finished1, int *ans1){
    // sys_move_cursor(0, 7);
    // printf("1 %d %d\n", *finished1, *ans1);
    for(int i=0;i<600;i++){
        sys_move_cursor(0, 7);
        printf("add1 thread is calculating *ans1+=data[%d]", i);
        *ans1+=data[i];
    }
    *finished1 = 1;
    while(1);
}

void add2(int *finished2, int *ans2){
    // sys_move_cursor(0, 8);
    // printf("2 %d %d\n", *finished2, *ans2);
    for(int i=600;i<1200;i++){
        sys_move_cursor(0, 8);
        printf("add2 thread is calculating *ans2+=data[%d]", i);
        *ans2+=data[i];
    }
    *finished2 = 1;
    while(1);
}

int main(void)
{
    int print_location = 6;
    volatile int finished1=0, finished2=0, ans1=0, ans2=0;
    for(int i=0;i<1200;i++){
        data[i]=i;
    }

    int32_t tid1,tid2;

    sys_thread_create(&tid1, (uint64_t)add1, (long)&finished1, (long)&ans1, 0L);
    sys_thread_create(&tid2, (uint64_t)add2, (long)&finished2, (long)&ans2, 0L);

    while(!finished1||!finished2);
    sys_move_cursor(0, print_location);
    printf("> [TASK] This task is to test thread_create. (%d)", ans1+ans2);
    while(1);
}
