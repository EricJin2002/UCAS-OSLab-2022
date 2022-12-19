#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef WIN32
#include <arpa/inet.h>
#include <pcap.h>
#else
#include <Windows.h>
#include "pcap.h"
#endif  // !WIN32

#include "common.h"

receiver_t g_receiver;

static char s_requests[] = "Requests: ";
static char s_response[] = "Response: ";

static void handle_packet(const char *packet, uint32_t pkt_len)
{
    int mode = g_config.mode;

    if (MODE_RECEIVE == mode)
    {
        g_receiver.recv_cnt += 1;
    }
    
    // Check the payload by comparing the payload
    // (Here the checksum is ignored, as students are not familiar with NW)
    struct iphdr *ip_hdr = packet_to_ip_hdr(packet);
    struct tcphdr *tcp_hdr = packet_to_tcp_hdr(packet);

    int hdr_len = ETHER_HDR_SIZE + IP_HDR_SIZE(ip_hdr) \
                + TCP_HDR_SIZE(tcp_hdr);

    if (MODE_SEND_RECEIVE == mode)
    {
        // Check whether this packet comes from the sender
        if (0 == memcmp(packet + hdr_len, s_requests, strlen(s_requests)))
        {
            return;
        }

        g_receiver.recv_cnt += 1;

        if (0 == memcmp(packet + hdr_len, s_response, strlen(s_response)))
        {
            // Next check its sequence number
            uint32_t pkt_seq = ntohl(tcp_hdr->seq);
            uint32_t pl_len = ntohs(ip_hdr->tot_len) - IP_HDR_SIZE(ip_hdr) \
                - TCP_HDR_SIZE(tcp_hdr);

            if (pkt_seq < g_receiver.next_seq)  // Duplicate Packet
            {
                g_receiver.dupl_cnt += 1;
            }
            else  // Normal or Out of order
            {
                // Get its index of the packet stream
                uint32_t pkt_idx = atoi(packet + hdr_len + strlen(s_response));
                printf("Info: Index of received packet is %u\n", pkt_idx);

                insert_rcv_ofo_buf(pkt_seq, pkt_idx, pl_len);
            }
        }
    }
    else if (MODE_RECEIVE_SEND == mode)  // Echo it back
    {
        // Check whether this packet comes from the receiver itself
        if (0 == memcmp(packet + hdr_len, s_response, strlen(s_response)))
        {
            return;
        }

        g_receiver.recv_cnt += 1;

        // Create a copy of the original packet
        char *new_pkt = (char *)malloc(pkt_len);
        if (NULL == new_pkt)
        {
            printf("[%s] Error: Cannot allocate memory for new_pkt!\n", \
                __FUNCTION__);
            exit(EXIT_FAILURE);
        }

        memcpy(new_pkt, packet, pkt_len);

        // Substitute the content of the payload
        memcpy(new_pkt + hdr_len, s_response, strlen(s_response));

        // Echo this packet back to the opposite
        if (0 != pcap_sendpacket(g_handle, (const u_char *)new_pkt, pkt_len))
        {
            printf("[%s] Error: Cannot send packets due to %s ...\n", \
                __FUNCTION__, pcap_geterr(g_handle));
        }
        else
        {
            g_receiver.echo_cnt += 1;
        }

        free(new_pkt);
    }

    if (0 == g_receiver.recv_cnt % g_config.rcv_pkt_interval)
    {
        print_results();
    }
}

#ifndef WIN32
void *receiver_thread(void *param)
#else
unsigned int WINAPI receiver_thread(void *param)
#endif  // !WIN32
{
    // Check whether the mode is to launch receiver
    if (0 == is_receiver_timer_mode(g_config.mode))
    {
        printf("[%s] Error: Cannot create receiver thread under mode %d\n", \
            __FUNCTION__, g_config.mode);
        exit(EXIT_FAILURE);
    }

    // Initialize the receiver
    g_receiver.recv_cnt = 0;
    g_receiver.dupl_cnt = 0;
    g_receiver.echo_cnt = 0;
    g_receiver.next_seq = 0;
    g_receiver.next_idx = 0;
    init_rcv_ofo_buf();

    // Start receiving packets
    struct pcap_pkthdr *pkthdr = NULL;
    const u_char *pkt_data = NULL;

    while (NULL != g_handle)
    {
        // Try to capture those coming packets
        int ret_val = pcap_next_ex(g_handle, &pkthdr, &pkt_data);

        if (ret_val < 0)
        {
            printf("[%s] Error: Cannot capture packets due to %s ...\n", \
                __FUNCTION__, pcap_geterr(g_handle));
            break;
        }
        else if (0 == ret_val)  // Timeout, retry
        {
            continue;
        }

        // Handle this packet
        handle_packet((const char *)pkt_data, (uint32_t)pkthdr->caplen);

        set_timer();
    }

#ifndef WIN32
    return NULL;
#else
    CloseHandle(g_receiver.pid);
    return 0;
#endif  // !WIN32
}