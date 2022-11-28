#include <e1000.h>
#include <type.h>
#include <os/sched.h>
#include <os/string.h>
#include <os/list.h>
#include <os/smp.h>

static LIST_HEAD(send_block_queue);
static LIST_HEAD(recv_block_queue);

int do_net_send(void *txpacket, int length)
{
    // TODO: [p5-task1] Transmit one network packet via e1000 device
    // TODO: [p5-task3] Call do_block when e1000 transmit queue is full
    int tail;
    if(!td_sendable(&tail)){
        do_block(&current_running_of[get_current_cpu_id()]->list, &send_block_queue, &pcb_lock);
    }

    return e1000_transmit(txpacket, length);

    // TODO: [p5-task4] Enable TXQE interrupt if transmit queue is full

    // return 0;  // Bytes it has transmitted
}

int do_net_recv(void *rxbuffer, int pkt_num, int *pkt_lens)
{
    // TODO: [p5-task2] Receive one network packet via e1000 device
    // TODO: [p5-task3] Call do_block when there is no packet on the way
    int ret=0;
    for(int i=0;i<pkt_num;i++){
        // Q: if multi processes call this func, what is the recv order ?
        int tail;
        if(!rd_recvable(&tail)){
            do_block(&current_running_of[get_current_cpu_id()]->list, &recv_block_queue, &pcb_lock);
        }

        pkt_lens[i] = e1000_poll(rxbuffer+ret);
        ret+=pkt_lens[i];
    }
    return ret;

    // return 0;  // Bytes it has received
}

// for [p5-task3]
void check_net_send(void){
    int tail;
    if(!list_is_empty(&send_block_queue) && td_sendable(&tail)){
        do_unblock(list_pop(&send_block_queue));
    }
}

// for [p5-task3]
void check_net_recv(void){
    int tail;
    if(!list_is_empty(&recv_block_queue) && rd_recvable(&tail)){
        do_unblock(list_pop(&recv_block_queue));
    }
}

void net_handle_irq(void)
{
    // TODO: [p5-task4] Handle interrupts from network device
}