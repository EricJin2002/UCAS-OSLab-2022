#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

typedef struct {
    int recved_signal;
    int sended_signal;
    int length;
    char buffer[2048];
} pack_t;

int main(){
    pack_t *shm = (pack_t *)sys_shmpageget(2022);
    int mutex = sys_mutex_init(2022);
    
    int recved = sys_condition_init(2022);
    int sended = sys_condition_init(2023);

    int cnt=0;

    sys_mutex_acquire(mutex);
    
    while(1){
        while(!shm->recved_signal){
            sys_condition_wait(recved,mutex);    
        }
        shm->recved_signal=0;

        for(int i=0;i+8<shm->length;i++){
            if(!strncmp(shm->buffer+i,"Requests:",9)){
                shm->buffer[i  ]='R';
                shm->buffer[i+1]='e';
                shm->buffer[i+2]='s';
                shm->buffer[i+3]='p';
                shm->buffer[i+4]='o';
                shm->buffer[i+5]='n';
                shm->buffer[i+6]='s';
                shm->buffer[i+7]='e';
                shm->buffer[i+8]=':';
            }
        }
        sys_net_send(shm->buffer,shm->length);
        sys_move_cursor(20,++cnt);
        printf("echo sended (%d)\n",shm->length);
        
        shm->sended_signal=1;
        sys_condition_signal(sended);
    }

    sys_mutex_release(mutex);
    
    sys_condition_destroy(recved);
    sys_condition_destroy(sended);
    sys_shmpagedt(shm);
}