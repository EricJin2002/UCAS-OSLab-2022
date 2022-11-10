#ifndef __INCLUDE_LOADER_H__
#define __INCLUDE_LOADER_H__

#include <type.h>

// for [p1-task4]
// copied from createimage.c
#define SECTOR_SIZE 512
#define NBYTES2SEC(nbytes) (((nbytes) / SECTOR_SIZE) + ((nbytes) % SECTOR_SIZE != 0))

// uint64_t load_task_img(int taskid);
// 
// // for [p1-task4]
// uint64_t load_task_img_via_name(char *taskname);
// void excute_task_img_via_name(char *taskname);

uint64_t load_app_img(int taskid, uintptr_t pgdir);
uint64_t load_app_img_via_name(char *taskname, uintptr_t pgdir);
int load_bat_img(int taskid);
int load_bat_img_via_name(char *taskname);
int find_task_named(char *taskname);

#endif