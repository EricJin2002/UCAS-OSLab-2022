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
        sys_net_recv(shm->buffer,1,&shm->length);
        sys_move_cursor(0,++cnt);
        printf("echo recved (%d)\n",shm->length);

        shm->recved_signal=1;
        sys_condition_signal(recved);

        while(!shm->sended_signal){
            sys_condition_wait(sended,mutex);
        }
        shm->sended_signal=0;
    }

    sys_mutex_release(mutex);

    sys_condition_destroy(recved);
    sys_condition_destroy(sended);
    sys_shmpagedt(shm);

}