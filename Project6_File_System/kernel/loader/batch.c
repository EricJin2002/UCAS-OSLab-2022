#include <os/loader.h>

void exec_batch(char *bat_cache){
    int bat_iter = 0;
    int bat_iter_his = 0;
    pcb_t *pcb_hist = 0;
    while(bat_cache[bat_iter]){
        if(bat_cache[bat_iter]=='\n'){
            bat_cache[bat_iter]='\0';
            pcb_hist = do_parse_and_exec_and_wait(bat_cache+bat_iter_his, pcb_hist);
            bat_iter_his=bat_iter+1;
        }
        bat_iter++;
    }
    do_parse_and_exec_and_wait(bat_cache+bat_iter_his, pcb_hist);
}
