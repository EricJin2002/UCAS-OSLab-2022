#ifndef __RECEIVER_H__
#define __RECEIVER_H__

#include <stdint.h>

#ifndef WIN32
#include <pthread.h>
#else
#include <Windows.h>
#endif  // !WIN32

#include "rcv_ofo_buf.h"

typedef struct {
#ifndef WIN32
    pthread_t pid;              // To start a receiver thread
#else
    HANDLE pid;                 // To start a receiver thread
#endif  // !WIN32
    int recv_cnt;               // Number of packets that receiver has received
    int dupl_cnt;               // Number of duplicate packets
    int echo_cnt;               // Number of packets echoed to the opposite (Used in MODE_RECEIVE_SEND)
    uint32_t next_seq;          // The seq of next packet expected to receive
    uint32_t next_idx;		    // The idx of next packet expected to receive
    ofo_buf_entry_t ofo_buf;    // Store out of order packets
#ifndef WIN32
    pthread_mutex_t ofo_mutex;  // To synchronize the access to ofo_buf  
#else
    HANDLE ofo_mutex;		    // To synchronize the access to ofo_buf    
#endif  // !WIN32
} receiver_t;

extern receiver_t g_receiver;

#ifndef WIN32
void *receiver_thread(void *param);
#else
unsigned int WINAPI receiver_thread(void *param);
#endif  // !WIN32

#endif  // !__RECEIVER_H__
