#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define PAGE_SIZE 4096
#define TEST_NUM 30

int main(){
    volatile int data[PAGE_SIZE*TEST_NUM];

    for(int i=0;i<TEST_NUM;i++){
        data[i*PAGE_SIZE]=i;
    }
    sys_move_cursor(0, 5);
    for(int i=0;i<TEST_NUM;i++){
        printf("%d ", data[i*PAGE_SIZE]);
    }
    printf("\n");

}