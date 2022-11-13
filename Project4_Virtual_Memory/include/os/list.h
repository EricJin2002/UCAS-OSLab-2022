/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * * * * * * * * Copyright (C) 2018 Institute of Computing
 * Technology, CAS Author : Han Shukai (email :
 * hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * * * * * * * * Changelog: 2019-8 Reimplement queue.h.
 * Provide Linux-style doube-linked list instead of original
 * unextendable Queue implementation. Luming
 * Wang(wangluming@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * * * * * * * */

#ifndef INCLUDE_LIST_H_
#define INCLUDE_LIST_H_

#include <type.h>

// double-linked list
typedef struct list_node
{
    struct list_node *next, *prev;
} list_node_t;

typedef list_node_t list_head;

// LIST_HEAD is used to define the head of a list.
#define LIST_HEAD(name) struct list_node name = {&(name), &(name)}

/* TODO: [p2-task1] implement your own list API */
static inline int list_is_empty(list_head *queue){
    return queue->next==queue;
}

static inline void list_push(list_head *queue, list_node_t *node){
    node->next=queue;
    node->prev=queue->prev;
    queue->prev = node;
    node->prev->next=node;
}

static inline list_node_t *list_pop(list_head *queue){
    if(list_is_empty(queue)) return 0;
    list_node_t *ret=queue->next;
    queue->next=queue->next->next;
    queue->next->prev=queue;
    ret->prev=ret;
    ret->next=ret;
    return ret;
}

static inline int list_delete(list_node_t *node){
    return list_pop(node->prev)?1:0;
}

// for [p3-task4]
// find a node satisfied the given constraint and pop it
static inline list_node_t *list_find_and_pop(list_head *queue, void *filter){
    for(list_node_t *node=queue->next;node!=queue;node=node->next){
        if((*(int (*)(list_node_t *))filter)(node)){
            return list_pop(node->prev);
        }
    }
    return 0;
}

// for [p4]
// find a node satisfied the given constraint
// if exsist, return 1; else, return 0 
static inline int list_find(list_head *queue, void *filter){
    for(list_node_t *node=queue->next;node!=queue;node=node->next){
        if((*(int (*)(list_node_t *))filter)(node)){
            return 1;
        }
    }
    return 0;
}

// for [p3]
#define STRUCT_OFFSET(struct_type, element) (unsigned long)&((struct struct_type *)0)->element

#endif
