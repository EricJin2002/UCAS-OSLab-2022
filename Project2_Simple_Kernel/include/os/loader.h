#ifndef __INCLUDE_LOADER_H__
#define __INCLUDE_LOADER_H__

#include <type.h>

// for [p1-task4]
// copied from createimage.c
#define SECTOR_SIZE 512
#define NBYTES2SEC(nbytes) (((nbytes) / SECTOR_SIZE) + ((nbytes) % SECTOR_SIZE != 0))

uint64_t load_task_img(int taskid);

// for [p1-task4]
uint64_t load_task_img_via_name(char *taskname);
void excute_task_img_via_name(char *taskname);

#endif