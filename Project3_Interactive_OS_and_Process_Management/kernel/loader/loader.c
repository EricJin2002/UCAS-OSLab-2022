#include <os/task.h>
#include <os/string.h>
#include <os/kernel.h>
#include <os/loader.h>
#include <type.h>
#include <os/sched.h> // for [p3-task1]

// return entrypoint for app and 0 for bat
uint64_t load_task_img(int taskid)
{
    /**
     * TODO:
     * 1. [p1-task3] load task from image via task id, and return its entrypoint
     * 2. [p1-task4] load task via task name, thus the arg should be 'char *taskname'
     */
    
    // for [p1-task3]
    /* bios_sdread(0x52000000 + 0x10000 * taskid, 15, 1 + 15 * (taskid + 1));
     * return 0x52000000 + 0x10000 * taskid;
     */

    if(!(taskid>=0&&taskid<task_num)){
        // no such taskid
        return 0;
    }

    if(tasks[taskid].type == app){
        // for [p1-task4]
        uint32_t block_id = tasks[taskid].offset/SECTOR_SIZE;
        uint32_t block_num = NBYTES2SEC(tasks[taskid].offset%SECTOR_SIZE + tasks[taskid].size);
        uint64_t mem_addr = tasks[taskid].entrypoint; //0x52000000 + 0x10000 * taskid;
        bios_sdread(mem_addr, block_num, block_id);
        // shift task entrypoint to the right address
        memcpy((uint8_t *)mem_addr, (uint8_t *)(mem_addr + tasks[taskid].offset%SECTOR_SIZE), tasks[taskid].size);
        return mem_addr;
    } else { // tasks[taskid].type == bat
        // load and excute batch for [p1-task5]

        printk("\n===Reading batch from image...===\n");

        // read batch content
        char bat_cache[1024]; //TODO: what if bat.txt is too big
        uint32_t bat_size = tasks[taskid].size;
        uint32_t bat_off = tasks[taskid].offset;
        uint16_t bat_block_id = bat_off/SECTOR_SIZE;
        uint16_t bat_block_num = NBYTES2SEC(bat_off%SECTOR_SIZE + bat_size);
        bios_sdread((unsigned int)(uint64_t)bat_cache, bat_block_num, bat_block_id);
        memcpy((uint8_t *)bat_cache, (uint8_t *)(uint64_t)(bat_cache + bat_off%SECTOR_SIZE), bat_size);
        printk(bat_cache);
        printk("\n===Finish reading!===\n");

        printk("\n");

        //excute batch
        printk("\n===Now excute batch!===\n");
        int bat_iter = 0;
        int bat_iter_his = 0;
        while(bat_cache[bat_iter]){
            if(bat_cache[bat_iter]=='\n'){
                bat_cache[bat_iter]='\0';
                // excute_task_img_via_name(bat_cache + bat_iter_his);
                // for [p3-task1]
                do_exec(bat_cache + bat_iter_his, 0, 0);
                bat_iter_his=bat_iter+1;
            }
            bat_iter++;
        }
        // excute_task_img_via_name(bat_cache + bat_iter_his);
        // for [p3-task1]
        do_exec(bat_cache + bat_iter_his, 0, 0);
        printk("===All tasks in batch are excuted!===\n");

        printk("\n");
    }
    
    return 0;
}

uint64_t load_task_img_via_name(char *taskname){
    int task_iter;
    // compare task name one by one
    for(task_iter=0; task_iter<task_num; task_iter++){
        if(strcmp(tasks[task_iter].name, taskname)==0){
            return load_task_img(task_iter);
        }
    }
    if(task_iter==task_num){
        if(*taskname=='\0'){
            printk("Task name empty!\n");
        }else{
            printk("No task named ");
            printk(taskname);
            printk("!\n");
        }
    }
    return 0;
}

void excute_task_img_via_name(char *taskname){
    uint64_t func;
    if((func=load_task_img_via_name(taskname)))
        ((void (*)())func)();
}