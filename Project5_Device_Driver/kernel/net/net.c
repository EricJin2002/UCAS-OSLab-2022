#include <e1000.h>
#include <type.h>
#include <os/sched.h>
#include <os/string.h>
#include <os/list.h>
#include <os/smp.h>
#include <assert.h>
#include <screen.h> // for [p5-task4]

static LIST_HEAD(send_block_queue);
static LIST_HEAD(recv_block_queue);

int do_net_send(void *txpacket, int length)
{
    // TODO: [p5-task1] Transmit one network packet via e1000 device
    // TODO: [p5-task3] Call do_block when e1000 transmit queue is full
    // TODO: [p5-task4] Enable TXQE interrupt if transmit queue is full
    int tail;
    if(!td_sendable(&tail)){
        e1000_write_reg(e1000, E1000_IMS, E1000_IMS_TXQE);
        local_flush_dcache();

        do_block(&current_running_of[get_current_cpu_id()]->list, &send_block_queue, &pcb_lock);

        if(list_is_empty(&send_block_queue)){
            e1000_write_reg(e1000, E1000_IMC, E1000_IMC_TXQE);
        }
    }

    return e1000_transmit(txpacket, length);

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
    if(!list_is_empty(&send_block_queue)){
        if(td_sendable(&tail)){
            do_unblock(list_pop(&send_block_queue));
        }else{
            printk("td not sendable!\n");
        }
    }
}

// for [p5-task3]
void check_net_recv(void){
    int tail;
    if(!list_is_empty(&recv_block_queue)){
        if(rd_recvable(&tail)){
            do_unblock(list_pop(&recv_block_queue));
        }else{
            printk("rd not recvable!\n");
        }
    }
}

void net_handle_irq(void)
{
    // TODO: [p5-task4] Handle interrupts from network device

    // for debug
    static int cnt0=0,cnt1=0,cnt2=0;
    printk("[in net_handle_irq] (%d)\n",cnt0++);

    uint32_t int_cause = e1000_read_reg(e1000, E1000_ICR);
    if(int_cause&E1000_ICR_TXQE){
        printk("check_net_send (%d)\n",cnt1++);
        check_net_send();
    }
    
    if(int_cause&E1000_ICR_RXDMT0){
        printk("check_net_recv (%d)\n",cnt2++);
        check_net_recv();
    }
}