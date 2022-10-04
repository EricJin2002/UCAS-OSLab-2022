#include <stdio.h>
#include <unistd.h>

static int data[1200];

void add1(int *finished1, int *ans1){
    // sys_move_cursor(0, 8);
    // printf("1 %d %d\n", *finished1, *ans1);
    for(int i=0;i<600;i++){
        *ans1+=data[i];
    }
    *finished1 = 1;
    while(1);
}

void add2(int *finished2, int *ans2){
    // sys_move_cursor(0, 9);
    // printf("2 %d %d\n", *finished2, *ans2);
    for(int i=600;i<1200;i++){
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

    sys_thread_create((uint64_t)add1, &finished1, &ans1, 0, 0);
    sys_thread_create((uint64_t)add2, &finished2, &ans2, 0, 0);

    while(!finished1||!finished2);
    sys_move_cursor(0, print_location);
    printf("> [TASK] This task is to test thread_create. (%d)", ans1+ans2);
    while(1);
}
