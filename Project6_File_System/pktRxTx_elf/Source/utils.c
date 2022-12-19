#include <stdio.h>
#include <stdlib.h>

#include "common.h"

static int count_lost_packets(ofo_buf_entry_t *sentry)
{
    platform_mutex_lock(&g_receiver.ofo_mutex);

    int lost_cnt = 0;
    uint32_t prev_idx = g_receiver.next_idx;
    ofo_buf_entry_t *entry_p = NULL;
    for (entry_p = sentry->next; entry_p != sentry; entry_p = entry_p->next)
    {
        lost_cnt += (int)(entry_p->idx - prev_idx);
        prev_idx = entry_p->idx;
    }

    platform_mutex_unlock(&g_receiver.ofo_mutex);

    return lost_cnt;
}

void print_results(void)
{
    int mode = g_config.mode;

    if (MODE_RECEIVE == mode)
    {
        printf("Info: Has received %d packets ...\n", g_receiver.recv_cnt);
    }
    else if (MODE_SEND_RECEIVE == mode)
    {
        // Traverse rcv_ofo_buf to get the number of lost packets
        int lost_cnt = count_lost_packets(&g_receiver.ofo_buf);

        // Dump the final results
        printf("Info: Has received %d packets:\n" \
            "\t%u packets are in sequence;\n" \
            "\t%d packets are lost;\n" \
            "\t%d packets are duplicate.\n", \
            g_receiver.recv_cnt, g_receiver.next_idx, \
            lost_cnt, g_receiver.dupl_cnt);
    }
    else if (MODE_RECEIVE_SEND == mode)
    {
        printf("Info: Has received %d packets, "\
            "and successfully echo %d packets back ...\n",
            g_receiver.recv_cnt, g_receiver.echo_cnt);
    }
}