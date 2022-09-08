#ifndef __INCLUDE_LOADER_H__
#define __INCLUDE_LOADER_H__

#include <type.h>

uint64_t load_task_img(int taskid);

// for [p1-task4]
void excute_task_img_via_name(char *taskname);

#endif