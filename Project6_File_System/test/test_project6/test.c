#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

typedef struct {
    int visited;
    char mbox_name[50];
    int done;
} shared_t;

shared_t *shared;
int barr;

static int global_cnt;

void thread_func(int tidx){
    // printf("son reach barr (%d)\n",++global_cnt);
    sys_barrier_wait(barr);

    int mbox=sys_mbox_open(shared->mbox_name);
    char msg[]="3.14159265358979323846264338327950288419716939937510";
    srand(tidx);

    int local_cnt=0;

    for(int i=0;i<10;i++){
        int msg_len = rand()%sizeof(msg);

        sys_move_cursor(0,tidx+2);
        printf("son %d  send msg_len (%d): %02d ", tidx, ++local_cnt, msg_len);
        
        sys_mbox_send(mbox, msg, msg_len);
        sys_sleep(1);
    }

    sys_thread_exit(NULL);

}

int main(int argc, char *argv[]){
    if(argc<3){
        printf("Not enough args!\n");
        return 0;
    }

    int key=atoi(argv[2]);

    shared=(shared_t *)sys_shmpageget(key);
    int is_father=0;
    if(!shared->visited){
        // first visit
        is_father=1;
        shared->visited=1;
    }

    if(is_father){
        // father

        strcpy(shared->mbox_name, argv[2]);
        int mbox=sys_mbox_open(shared->mbox_name);

        int fd=sys_fopen(argv[1], O_RDWR);
        
        sys_exec("test", argc, argv);

        int local_cnt=0;

        static char buffer[1000];
        int buff_size=atoi(argv[2]);
        while(!shared->done || sys_mbox_bytes(mbox)>=buff_size){
            while(sys_mbox_bytes(mbox)<buff_size){
                if(shared->done && sys_mbox_bytes(mbox)<buff_size){
                    goto done;
                }
            }

            sys_mbox_recv(mbox, buffer, buff_size);

            sys_move_cursor(0,1);
            printf("father recv msg_len (%d): %02d ", ++local_cnt ,buff_size);

            char time_str[20];
            int time_num = sys_get_tick();
            itoa(time_num, time_str, 20, 10);
            sys_fwrite(fd, time_str, 20);
            sys_fwrite(fd, "\n ", 2);
        }

    done:
        sys_fclose(fd);
        sys_mbox_close(mbox);
        sys_shmpagedt(shared);

    }else{
        // child

        barr=sys_barrier_init(key, 8);

        int32_t tid[8];
        for(int i=0;i<8;i++){
            sys_thread_create(tid+i, thread_func, i, NULL, NULL);
        }

        for(int i=0;i<8;i++){
            int ret;
            sys_thread_join(tid[i], &ret);
        }

        shared->done=1;
    }

}