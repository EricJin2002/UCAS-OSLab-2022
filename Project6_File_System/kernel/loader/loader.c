#include <os/task.h>
#include <os/string.h>
#include <os/kernel.h>
#include <os/loader.h>
#include <type.h>
#include <os/sched.h>   // for [p3-task1]
#include <os/mm.h>      // for [p4-task1]
#include <assert.h>     // for [p4-task1]
#include <pgtable.h>    // for [p4-task1]

// for [p3-task1]
// copied from shell.c
#define TASK_NAME_MAXLEN    50
static inline int isspace(int ch){
    return ch == ' '  || ch == '\t' || ch == '\n' ||
           ch == '\v' || ch == '\f' || ch == '\r';
}
static void parse_args(char *cache, int *argc, char *(*argv)[TASK_NAME_MAXLEN+1]){
    *argc=0;
    int parsing=0;
    for(int head=0;cache[head];head++){
        if(isspace(cache[head])){
            parsing=0;
            cache[head]='\0';
            continue;
        }
        if(!parsing){
            (*argv)[(*argc)++]=cache+head;
            parsing=1;
        }
    }
    (*argv)[*argc]=(char *)0;
}
static pcb_t *do_parse_and_exec_and_wait(char *cache, pcb_t *waiton){
    int argc;
    char *argv[TASK_NAME_MAXLEN+1];
    parse_args(cache, &argc, &argv);
    
    if(!argc){
        return waiton;
    }

    int wait_end = 1;
    if(!strcmp(argv[argc-1],"&")){
        wait_end = 0;
        argc--;
        argv[argc]=(char *)0;
    }
        
    if(!argc){
        return waiton;
    }
    
    // allow comment
    if(!strcmp(argv[0],"#")){
        return waiton;
    }

    pid_t pid=do_exec(argv[0], argc, argv);
    
    if(!pid){
        return waiton;
    }

    pcb_t *ret;
    for(int i=1;i<process_id;i++){
        if(pcb[i].pid==pid){
            ret=pcb+i;
            break;
        }
    }
    if(waiton){
        list_delete(&ret->list);
        list_push(&waiton->wait_list, &ret->list);
        ret->status = TASK_BLOCKED;
    }

    return wait_end ? ret : 0;
}

// return entrypoint(va)
uint64_t load_app_img(int taskid, pcb_t *owener_pcb){
    printl("[in load_app_img]\n");
    assert(taskid>=0 && taskid<task_num);
    assert(tasks[taskid].type==app);

    // if (!tasks[taskid].loaded){
    //     tasks[taskid].loaded = 1;
    //     // for [p1-task4]
    //     uint32_t block_id = tasks[taskid].offset/SECTOR_SIZE;
    //     uint32_t block_num = NBYTES2SEC(tasks[taskid].offset%SECTOR_SIZE + tasks[taskid].size);
    //     uint64_t mem_addr = tasks[taskid].entrypoint; //0x52000000 + 0x10000 * taskid;
    //     bios_sdread(mem_addr, block_num, block_id);
    //     // shift task entrypoint to the right address
    //     memcpy((uint8_t *)mem_addr, (uint8_t *)(mem_addr + tasks[taskid].offset%SECTOR_SIZE), tasks[taskid].size);
    //     return mem_addr;
    // } else {
    //     // for [p3-task3]
    //     // if already loaded, don't load again
    //     return tasks[taskid].entrypoint;
    // }

    // for [p4-task1]

    uint32_t block_id = tasks[taskid].offset/SECTOR_SIZE;
    uint32_t block_num = NBYTES2SEC(tasks[taskid].offset%SECTOR_SIZE + tasks[taskid].size);
    uint64_t mem_va = tasks[taskid].entrypoint;
    int needed_page_num = (tasks[taskid].memsz - 1 + PAGE_SIZE) / PAGE_SIZE;
    uint64_t offset_in_sector = tasks[taskid].offset%SECTOR_SIZE;
    uint64_t mem_kva=0;
    static char sdread_buff[2*PAGE_SIZE];

    // the `<=` in the next line MUST be preserved !!
    // the extra page is used to transfer offseted data
    // due to the lack of padding / alignment in img
    for(int i=0;i<=needed_page_num;i++){
        // alloc a new page
        mem_kva = alloc_page_helper(mem_va, owener_pcb);

        // read sd
        bios_sdread(
            kva2pa((uintptr_t)sdread_buff), 
            2*PAGE_SIZE/SECTOR_SIZE,
            block_id
        );

        // shift task entrypoint to the right address
        memcpy(
            (uint8_t *)(mem_kva), 
            (uint8_t *)(sdread_buff + offset_in_sector), 
            PAGE_SIZE
        );

        block_id += PAGE_SIZE/SECTOR_SIZE;
        mem_va += PAGE_SIZE;
    }

    printl("\n");
    return tasks[taskid].entrypoint;
}

// on success, return entrypoint(va); else, return -1
uint64_t load_app_img_via_name(char *taskname, pcb_t *owener_pcb){
    int taskid = find_task_named(taskname);
    if(taskid!=-1 && tasks[taskid].type==app){
        return load_app_img(taskid, owener_pcb);
    }else{
        return -1;
    }
}

int load_bat_img(int taskid){
    assert(taskid>=0 && taskid<task_num);
    assert(tasks[taskid].type==bat);

    // for [p3-task3]
    // todo: considering the bug that subcore cannot sd_read, we must ensure only the main core can exec bat
    // currently, it has possibility of malfunctioning if sub cores call bat

    // load and excute batch for [p1-task5]

    printk("\n===Reading batch from image...===\n");

    // read batch content
    // fixme: the buffer is too big that shell's stack ~~may~~ overflow
    // note: we cannot change the `bat_cache` to static, because bat can call bat
    char bat_cache[1024]; //todo: what if bat.txt is too big
    uint32_t bat_size = tasks[taskid].size;
    uint32_t bat_off = tasks[taskid].offset;
    uint16_t bat_block_id = bat_off/SECTOR_SIZE;
    uint16_t bat_block_num = NBYTES2SEC(bat_off%SECTOR_SIZE + bat_size);
    bios_sdread((unsigned int)(uint64_t)kva2pa((uintptr_t)bat_cache), bat_block_num, bat_block_id);
    memcpy((uint8_t *)bat_cache, (uint8_t *)(uint64_t)(bat_cache + bat_off%SECTOR_SIZE), bat_size);
    printk(bat_cache);
    printk("\n===Finish reading!===\n");

    printk("\n");

    //excute batch
    printk("\n===Now excute batch!===\n");
    int bat_iter = 0;
    int bat_iter_his = 0;
    pcb_t *pcb_hist = 0;
    while(bat_cache[bat_iter]){
        if(bat_cache[bat_iter]=='\n'){
            bat_cache[bat_iter]='\0';
            // excute_task_img_via_name(bat_cache + bat_iter_his);
            // for [p3-task1]
            pcb_hist = do_parse_and_exec_and_wait(bat_cache+bat_iter_his, pcb_hist);
            bat_iter_his=bat_iter+1;
        }
        bat_iter++;
    }
    // excute_task_img_via_name(bat_cache + bat_iter_his);
    // for [p3-task1]
    do_parse_and_exec_and_wait(bat_cache+bat_iter_his, pcb_hist);
    printk("===All tasks in batch are excuted!===\n");

    printk("\n");
    return 0;
}

// on fail, return -1
int load_bat_img_via_name(char *taskname){
    int taskid = find_task_named(taskname);
    if(taskid!=-1 && tasks[taskid].type==bat){
        return load_bat_img(taskid);
    }else{
        return -1;
    }
}


// find task named `taskname` in tasks array
// on found, return taskid; else, return -1
int find_task_named(char *taskname){
    int task_iter;
    // compare task name one by one
    for(task_iter=0; task_iter<task_num; task_iter++){
        if(strcmp(tasks[task_iter].name, taskname)==0){
            return task_iter;
        }
    }
    if(task_iter==task_num){
        if(*taskname=='\0'){
            printk("[kernel] Task name empty!\n");
        }else{
            printk("[kernel] No task named ");
            printk(taskname);
            printk("!\n");
        }
    }
    return -1;
}

// void excute_task_img_via_name(char *taskname){
//     uint64_t func;
//     if((func=load_task_img_via_name(taskname)))
//         ((void (*)())func)();
// }