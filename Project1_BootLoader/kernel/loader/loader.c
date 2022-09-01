#include <os/task.h>
#include <os/string.h>
#include <os/bios.h>
#include <type.h>

uint64_t load_task_img(int taskid)
{
    /**
     * TODO:
     * 1. [p1-task3] load task from image via task id, and return its entrypoint
     * 2. [p1-task4] load task via task name, thus the arg should be 'char *taskname'
     */
    bios_sdread(0x52000000 + 0x10000 * taskid, 15, 1 + 15 * (taskid + 1));
    return 0x52000000 + 0x10000 * taskid;

    return 0;
}