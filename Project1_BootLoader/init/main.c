#include <common.h>
#include <asm.h>
#include <os/bios.h>
#include <os/task.h>
#include <os/string.h>
#include <os/loader.h>
#include <type.h>

#define VERSION_BUF 50

// for [p1-task4]
// copied from createimage.c
#define SECTOR_SIZE 512
#define NBYTES2SEC(nbytes) (((nbytes) / SECTOR_SIZE) + ((nbytes) % SECTOR_SIZE != 0))

int version = 2; // version must between 0 and 9
char buf[VERSION_BUF];

// Task info array
task_info_t tasks[TASK_MAXNUM];

// for [p1-task4]
uint16_t task_num;

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
    memcpy(tasks, tasks_mem_ptr, TASK_MAXNUM * sizeof(task_info_t));

    // for [p1-task3] & [p1-task4]
    // read task_num
    task_num = *(uint16_t *)0x502001fe;
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
    
    bios_putstr("\n\r");

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


    // load and excute batch for [p1-task5]

    bios_putstr("Reading batch from image...\n\r======================\n\r");

    // read batch size
    uint32_t app_info_off = *(uint32_t *)(0x50200200 - 0xc);
    uint32_t bat_size_off = app_info_off + sizeof(task_info_t)*TASK_MAXNUM;
    uint16_t bat_size_block_id = bat_size_off/SECTOR_SIZE;
    uint16_t bat_size_block_num = NBYTES2SEC(bat_size_off%SECTOR_SIZE + 4);
    bios_sdread(0x52000000, bat_size_block_num, bat_size_block_id);
    uint32_t bat_size = *(uint32_t *)(0x52000000 + bat_size_off%SECTOR_SIZE);
    
    // read batch content
    char bat_cache[1024]; //TODO: what if bat.txt is too big
    uint32_t bat_off = bat_size_off + 4;
    uint16_t bat_block_id = bat_off/SECTOR_SIZE;
    uint16_t bat_block_num = NBYTES2SEC(bat_off%SECTOR_SIZE + bat_size);
    bios_sdread(0x52000000, bat_block_num, bat_block_id);
    memcpy(bat_cache, 0x52000000 + bat_off%SECTOR_SIZE, bat_size);
    bios_putstr(bat_cache);
    bios_putstr("\n\r======================\n\rFinish reading!\n\r");

    bios_putstr("\n\r");

    //excute batch
    bios_putstr("\n\rNow excute batch!\n\r======================\n\r");
    int bat_iter = 0;
    int bat_iter_his = 0;
    while(bat_cache[bat_iter]){
        if(bat_cache[bat_iter]=='\n'){
            bat_cache[bat_iter]='\0';
            excute_task_img_via_name(bat_cache + bat_iter_his);
            bat_iter_his=bat_iter+1;
        }
        bat_iter++;
    }
    excute_task_img_via_name(bat_cache + bat_iter_his);
    bios_putstr("======================\n\rAll tasks in batch are excuted!\n\r");

    bios_putstr("\n\r");

    // load and excute tasks by name for [p1-task4]
    bios_putstr("What to do next?\n\r");
    char cache[20];
    int head=0;
    int ch;
    while((ch=bios_getchar())){
        if(ch!=-1){
            if(ch=='\r'){// \r for Carriage Return and \n for Line Feed
                bios_putstr("\n\r");
                cache[head]='\0';
                excute_task_img_via_name(cache);
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
