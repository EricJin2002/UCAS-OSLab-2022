#ifndef __INCLUDE_TASK_H__
#define __INCLUDE_TASK_H__

#include <type.h>

#define TASK_MEM_BASE    0x52000000
#define TASK_MAXNUM      30
#define TASK_SIZE        0x10000

// for [p1-task5]
typedef enum{
    app, bat
} TYPE;

/* TODO: [p1-task4] implement your own task_info_t! */
typedef struct {
    char name[32];
    int offset;
    int size;
    uint64_t entrypoint;
    TYPE type;  // for [p1-task5]
    int loaded; // for [p3-task3] // due to the bug that subcore cannot sd_read, we must load all the tasks fisrt
} task_info_t;

extern task_info_t tasks[TASK_MAXNUM];

// for [p1-task4]
extern uint16_t task_num;

#endif