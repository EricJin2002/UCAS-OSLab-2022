#include <os/lock.h>
#include <os/sched.h>
#include <os/list.h>
#include <atomic.h>
#include <assert.h> // for [p3]
#include <os/string.h>

mailbox_t mboxs[MBOX_NUM];

void init_mbox(){
    for(int i=0;i<MBOX_NUM;i++){
        spin_lock_init(&mboxs[i].lock);
        mboxs[i].handle_num=0;
        mboxs[i].block_queue.next=&mboxs[i].block_queue;
        mboxs[i].block_queue.prev=&mboxs[i].block_queue;
        // memset(mboxs->buff, 0, sizeof(mboxs->buff));
        mboxs[i].head=mboxs[i].tail=0;
    }
}

int do_mbox_open(char *name){
    for(int i=0;i<MBOX_NUM;i++){
        spin_lock_acquire(&mboxs[i].lock);
        if(mboxs[i].handle_num>0 && !strcmp(mboxs[i].name, name)){
            mboxs[i].handle_num++;
            spin_lock_release(&mboxs[i].lock);
            return i;
        }
        spin_lock_release(&mboxs[i].lock);
    }
    for(int i=0;i<MBOX_NUM;i++){
        spin_lock_acquire(&mboxs[i].lock);
        if(mboxs[i].handle_num==0){
            mboxs[i].handle_num++;
            assert(mboxs[i].head==0);
            assert(mboxs[i].tail==0);
            assert(list_is_empty(&mboxs[i].block_queue));
            strcpy(mboxs[i].name, name);
            spin_lock_release(&mboxs[i].lock);
            return i;
        }
        spin_lock_release(&mboxs[i].lock);
    }
    return -1;
}

void do_mbox_close(int mbox_idx){
    spin_lock_acquire(&mboxs[mbox_idx].lock);
    mboxs[mbox_idx].handle_num=0;
    list_node_t *node;
    while((node=list_pop(&mboxs[mbox_idx].block_queue))){
        do_unblock(node);
    }
    mboxs[mbox_idx].head=mboxs[mbox_idx].tail=0;
    spin_lock_release(&mboxs[mbox_idx].lock);
}

int do_mbox_send(int mbox_idx, void * msg, int msg_length){
    spin_lock_acquire(&mboxs[mbox_idx].lock);
    while(mboxs[mbox_idx].head+msg_length>mboxs[mbox_idx].tail+MAX_MBOX_LENGTH){
        // not enough room to store
        do_block(&current_running->list,&mboxs[mbox_idx].block_queue,&mboxs[mbox_idx].lock);
    }
    
    // msg copy
    for(int i=0;i<msg_length;i++){
        mboxs[mbox_idx].buff[mboxs[mbox_idx].head%(MAX_MBOX_LENGTH+1)]=i[(char *)msg];
        mboxs[mbox_idx].head++;
    }
    
    list_node_t *node;
    if((node=list_pop(&mboxs[mbox_idx].block_queue))){
        do_unblock(node);
    }

    spin_lock_release(&mboxs[mbox_idx].lock);
    return 1;
}

int do_mbox_recv(int mbox_idx, void * msg, int msg_length){
    spin_lock_acquire(&mboxs[mbox_idx].lock);
    while(mboxs[mbox_idx].tail+msg_length>mboxs[mbox_idx].head){
        // not enough data to load
        do_block(&current_running->list,&mboxs[mbox_idx].block_queue,&mboxs[mbox_idx].lock);
    }
    
    // msg copy
    for(int i=0;i<msg_length;i++){
        i[(char *)msg]=mboxs[mbox_idx].buff[mboxs[mbox_idx].tail%(MAX_MBOX_LENGTH+1)];
        mboxs[mbox_idx].tail++;
    }
    
    list_node_t *node;
    if((node=list_pop(&mboxs[mbox_idx].block_queue))){
        do_unblock(node);
    }

    spin_lock_release(&mboxs[mbox_idx].lock);
    return 1;
}