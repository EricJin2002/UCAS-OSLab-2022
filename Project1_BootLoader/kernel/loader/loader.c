#include <os/task.h>
#include <os/string.h>
#include <os/bios.h>
#include <type.h>

// for [p1-task4]
// copied from createimage.c
#define SECTOR_SIZE 512
#define NBYTES2SEC(nbytes) (((nbytes) / SECTOR_SIZE) + ((nbytes) % SECTOR_SIZE != 0))

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

    // for [p1-task4]
    uint32_t block_id = tasks[taskid].offset/SECTOR_SIZE;
    uint32_t block_num = NBYTES2SEC(tasks[taskid].offset%SECTOR_SIZE + tasks[taskid].size);
    uint64_t mem_addr = tasks[taskid].entrypoint; //0x52000000 + 0x10000 * taskid;
    bios_sdread(mem_addr, block_num, block_id);
    // shift task entrypoint to the right address
    memcpy(mem_addr, mem_addr + tasks[taskid].offset%SECTOR_SIZE, tasks[taskid].size);
    return mem_addr;

    return 0;
}

void excute_task_img_via_name(char *taskname){
    int task_iter;
    // compare task name one by one
    for(task_iter=0; task_iter<task_num; task_iter++){
        if(strcmp(tasks[task_iter].name, taskname)==0){
            ((void (*)())load_task_img(task_iter))();
            break;
        }
    }
    if(task_iter==task_num){
        if(*taskname=='\0'){
            bios_putstr("Task name empty!\n\r");
        }else{
            bios_putstr("No task named ");
            bios_putstr(taskname);
            bios_putstr("!\n\r");
        }
    }
}