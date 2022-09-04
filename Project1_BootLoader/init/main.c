#include <common.h>
#include <asm.h>
#include <os/bios.h>
#include <os/task.h>
#include <os/string.h>
#include <os/loader.h>
#include <type.h>

#define VERSION_BUF 50

int version = 2; // version must between 0 and 9
char buf[VERSION_BUF];

// Task info array
task_info_t tasks[TASK_MAXNUM];

static int bss_check(void)
{
    for (int i = 0; i < VERSION_BUF; ++i)
    {
        if (buf[i] != 0)
        {
            return 0;
        }
    }
    return 1;
}

static void init_bios(void)
{
    volatile long (*(*jmptab))() = (volatile long (*(*))())BIOS_JMPTAB_BASE;

    jmptab[CONSOLE_PUTSTR]  = (long (*)())port_write;
    jmptab[CONSOLE_PUTCHAR] = (long (*)())port_write_ch;
    jmptab[CONSOLE_GETCHAR] = (long (*)())port_read_ch;
    jmptab[SD_READ]         = (long (*)())sd_read;
}

static void init_task_info(void)
{
    // TODO: [p1-task4] Init 'tasks' array via reading app-info sector
    // NOTE: You need to get some related arguments from bootblock first
    uint32_t app_info_off = *(uint32_t *)(0x50200200 - 0xc);
    task_info_t *tasks_mem_ptr = (task_info_t *)(0x52000000 + app_info_off%0x200);
    // copy app_info from mem to bss
    // since mem will be overwritten by the first app
    for(int i=0; i<TASK_MAXNUM; i++){
        strcpy(tasks[i].name, tasks_mem_ptr[i].name);
        tasks[i].offset = tasks_mem_ptr[i].offset;
        tasks[i].size = tasks_mem_ptr[i].size;
    }
}

int main(void)
{
    // Check whether .bss section is set to zero
    int check = bss_check();

    // Init jump table provided by BIOS (ΦωΦ)
    init_bios();

    // Init task information (〃'▽'〃)
    init_task_info();

    // Output 'Hello OS!', bss check result and OS version
    char output_str[] = "bss check: _ version: _\n\r";
    char output_val[2] = {0};
    int i, output_val_pos = 0;

    output_val[0] = check ? 't' : 'f';
    output_val[1] = version + '0';
    for (i = 0; i < sizeof(output_str); ++i)
    {
        buf[i] = output_str[i];
        if (buf[i] == '_')
        {
            buf[i] = output_val[output_val_pos++];
        }
    }

    bios_putstr("Hello OS!\n\r");
    bios_putstr(buf);

    // TODO: Load tasks by either task id [p1-task3] or task name [p1-task4],
    //   and then execute them.
    
    // for [p1-task2]
    /*
     * int ch;
     * while((ch=bios_getchar())){
     *     if(ch!=-1){
     *         if(ch=='\r'){
     *             // \r for Carriage Return and \n for Line Feed
     *             bios_putstr("\n\r");
     *         }else
     *             bios_putchar(ch);
     *     }
     * }
     */
    

    // for [p1-task3]
    // read task_num
    void *bootblock_end_loc = (void *)0x50200200;
    void *task_num_loc = bootblock_end_loc - 2;
    uint16_t task_num = *(uint16_t *)task_num_loc;

    // load and excute tasks by id for [p1-task3]
    /* int ch;
     * while((ch=bios_getchar())){
     *     if(ch!=-1){
     *         bios_putchar(ch);
     *         bios_putstr("\n\r");
     *         if(ch<'0'+task_num && ch>='0'){
     *             ((void (*)())load_task_img(ch-'0'))();
     *         }else{
     *             bios_putstr("Invalid task id!\n\r");
     *         }
     *     }
     * }
     */
    
    // load and excute tasks by name for [p1-task4]
    char cache[20];
    int head=0;
    int ch;
    while((ch=bios_getchar())){
        if(ch!=-1){
            if(ch=='\r'){// \r for Carriage Return and \n for Line Feed
                bios_putstr("\n\r");
                cache[head]='\0';
                int task_iter;
                // compare task name one by one
                for(task_iter=0; task_iter<task_num; task_iter++){
                    if(strcmp(tasks[task_iter].name, cache)==0){
                        ((void (*)())load_task_img(task_iter))();
                        break;
                    }
                }
                if(task_iter==task_num){
                    bios_putstr("No task named ");
                    bios_putstr(cache);
                    bios_putstr("!\n\r");
                }
                head=0;
            }else{
                if(head<20){
                    bios_putchar(ch);
                    cache[head++]=ch;
                }else{
                    bios_putstr("\n\rMaximal input reached!\n\r");
                    head=0;
                }
            }
        }
    }

    // Infinite while loop, where CPU stays in a low-power state (QAQQQQQQQQQQQ)
    while (1)
    {
        asm volatile("wfi");
    }

    return 0;
}
