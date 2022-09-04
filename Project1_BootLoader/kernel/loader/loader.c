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
    uint64_t mem_addr = 0x52000000 + 0x10000 * taskid;
    bios_sdread(mem_addr, block_num, block_id);
    // shift task entrypoint to the right address
    memcpy(mem_addr, mem_addr + tasks[taskid].offset%SECTOR_SIZE, tasks[taskid].size);
    return mem_addr;

    return 0;
}