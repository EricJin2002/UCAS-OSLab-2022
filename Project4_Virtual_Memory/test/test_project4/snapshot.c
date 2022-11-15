#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main(){
    // init
    int *data=0x40000000;
    for(int i=0;i<10;i++){
        data[i]=0;
    }
    int *snapshot_va[10];
    sys_move_cursor(0,1);

    printf("[data       pa %lx] ",sys_get_pa(data));
    for(int i=0;i<10;i++){
        printf("%d ",data[i]);
    }
    printf("\n");

    // test 0
    printf("--------------------------------------------\n");
    data[0]++;
    snapshot_va[0] = take_snapshot(data);
    data[1]++;

    printf("[data       pa %lx] ",sys_get_pa(data));
    for(int i=0;i<10;i++){
        printf("%d ",data[i]);
    }
    printf("\n");

    printf("[snapshot 0 pa %lx] ",sys_get_pa(snapshot_va[0]));
    for(int i=0;i<10;i++){
        printf("%d ",snapshot_va[0][i]);
    }
    printf("\n");

    //test 1
    printf("--------------------------------------------\n");
    data[2]++;
    snapshot_va[1] = take_snapshot(data);
    snapshot_va[2] = take_snapshot(data);
    data[3]++;

    printf("[data       pa %lx] ",sys_get_pa(data));
    for(int i=0;i<10;i++){
        printf("%d ",data[i]);
    }
    printf("\n");

    printf("[snapshot 0 pa %lx] ",sys_get_pa(snapshot_va[0]));
    for(int i=0;i<10;i++){
        printf("%d ",snapshot_va[0][i]);
    }
    printf("\n");

    printf("[snapshot 1 pa %lx] ",sys_get_pa(snapshot_va[1]));
    for(int i=0;i<10;i++){
        printf("%d ",snapshot_va[1][i]);
    }
    printf("\n");

    printf("[snapshot 2 pa %lx] ",sys_get_pa(snapshot_va[2]));
    for(int i=0;i<10;i++){
        printf("%d ",snapshot_va[2][i]);
    }
    printf("\n");
}