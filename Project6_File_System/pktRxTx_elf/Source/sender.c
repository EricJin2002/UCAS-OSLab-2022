#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef WIN32
#include <arpa/inet.h>
#include <sys/time.h>
#else
#include <time.h>
#include <Windows.h>
#include "pcap.h"
#endif  // !WIN32

#include "common.h"

sender_t g_sender;
FILE *fp;
char fname[16];
int cnt;
int sz;

static char s_requests[] = "Requests: ";

static int fill_payload_first(char *payload, int limit, uint32_t idx)
{
    if (!fp)
    {
        printf("> Please input file name: ");
        scanf("%s", fname);
        fp = fopen(fname, "r");
        printf("> Opening '%s'.\n", fname);
        fseek(fp, 0, SEEK_END);
        sz = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        printf("> Size is %dB\n", sz);   
    }
    char *buf = (char *)malloc(limit * sizeof(char));
    if (NULL == buf)
    {
        printf("[%s] Error: Cannot allocate more memory for payload!\n", \
                __FUNCTION__);
        exit(EXIT_FAILURE);
    }
    char sz_str[16];
    sprintf(sz_str,"%d", ((sz + 511) >> 9));
    memcpy(payload, sz_str, 16);
    memcpy(payload + 16, fname, sizeof(fname));
    printf("  [INFO] sending head packet, size = %dB.\n", sizeof(payload));
    return ((sz + 511) >> 9);
}

static void fill_payload(char *payload, int limit, uint32_t idx)
{
    char *buf = (char *)malloc(limit * sizeof(char));
    if (NULL == buf)
    {
        printf("[%s] Error: Cannot allocate more memory for payload!\n", \
                __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    int sz = fread(buf, 1, 512, fp);
    printf("  [INFO] sending %d packet, size = %dB.\n", cnt++, sz);
    memcpy(payload, buf, 512/*strlen(buf)*/);
}

static int send_packet(const char *payload, uint32_t seq, uint32_t ack)
{
    // Create a new packet
    int hdr_len = ETHER_HDR_SIZE + IP_BASE_HDR_SIZE + TCP_BASE_HDR_SIZE;
    int pl_len = 512;//strlen(payload); 
    int pkt_len = hdr_len + pl_len;

    char *pkt = (char *)malloc(pkt_len);
    if (NULL == pkt)
    {
        printf("[%s] Error: Cannot allocate for a new packet!\n", \
            __FUNCTION__);
        return 0;  // Which means having sent 0 bytes successfully
    }

    memset(pkt, 0, pkt_len);

    // Assembly ether header
    struct ethhdr *eth_hdr = packet_to_ether_hdr(pkt);

    memcpy(eth_hdr->ether_dhost, g_config.dst_mac, ETH_ALEN);
    memcpy(eth_hdr->ether_shost, g_config.src_mac, ETH_ALEN);
    eth_hdr->ether_type = htons(ETH_P_IP);

    // Assembly ip header
    struct iphdr *ip_hdr = packet_to_ip_hdr(pkt);

    ip_hdr->ihl = 5;
    ip_hdr->version = 4;
    ip_hdr->tos = 0;
    ip_hdr->tot_len = htons(IP_BASE_HDR_SIZE + TCP_BASE_HDR_SIZE + pl_len);
    ip_hdr->id = htons(54321);
    ip_hdr->frag_off = htons(IP_DF);
    ip_hdr->ttl = DEFAULT_TTL;
    ip_hdr->protocol = IPPROTO_TCP;
    ip_hdr->saddr = htonl(g_config.saddr);
    ip_hdr->daddr = htonl(g_config.daddr);

    // Assembly tcp header and payload
    struct tcphdr *tcp_hdr = packet_to_tcp_hdr(pkt);

    tcp_hdr->sport = htons(g_config.sport);
    tcp_hdr->dport = htons(g_config.dport);
    tcp_hdr->seq = htonl(seq);
    tcp_hdr->ack = htonl(ack);
    tcp_hdr->off = TCP_HDR_OFFSET;
    tcp_hdr->flags = TCP_PSH | TCP_ACK;
    tcp_hdr->rwnd = htons(TCP_DEFAULT_WINDOW);

    memcpy(pkt + hdr_len, payload, pl_len);

    // Set checksums
    tcp_hdr->checksum = tcp_checksum(ip_hdr, tcp_hdr);
    ip_hdr->checksum = ip_checksum(ip_hdr);

    // Send the packet out
    if (0 != pcap_sendpacket(g_handle, (const u_char *)pkt, pkt_len))
    {
        printf("[%s] Error: Cannot send packets due to %s ...\n", \
            __FUNCTION__, pcap_geterr(g_handle));

        free(pkt);
        return 0;
    }

    free(pkt);

    return pl_len;
}

static void send_packet_one_epoch(char *payload, int interval)
{
    fill_payload(payload, MAX_PAYLOAD_LEN, g_sender.send_cnt);
    int sent_bytes = send_packet(payload, g_sender.seq, 0);
    g_sender.send_cnt += (int)(sent_bytes > 0);
    g_sender.seq += sent_bytes;
    platform_wait_us(interval);
}

static int send_first_packet(char *payload, int interval)
{
    int sz = fill_payload_first(payload, MAX_PAYLOAD_LEN, g_sender.send_cnt);
    int sent_bytes = send_packet(payload, g_sender.seq, 0);
    g_sender.seq += sent_bytes;
    platform_wait_us(interval);
    return sz;
}

static void sender_send(char *input)
{
    char payload[MAX_PAYLOAD_LEN] = {'\0'};

    int interval = (g_config.interval > 0) ? g_config.interval: 
                                             INTERVAL_SEND;
    int num_pkts = send_first_packet(payload, interval);

    while ((int)g_sender.send_cnt < num_pkts)
    {
        send_packet_one_epoch(payload, interval);
    }
}

static void sender_test(char *input)
{
    char payload[MAX_PAYLOAD_LEN] = {'\0'};
    int interval = (g_config.interval > 0) ? g_config.interval:
                                            INTERVAL_BENCHMARK;
#ifndef WIN32
    long test_time = ((long)atoi(input + 5)) * 1000000;  // us
    long diff_time = 0;

    struct timeval tv_start;
    struct timeval tv_end;

    gettimeofday(&tv_start, NULL);

    while (1)
    {
        gettimeofday(&tv_end, NULL);

        diff_time = (tv_end.tv_sec - tv_start.tv_sec) * 1000000 \
                  + tv_end.tv_usec - tv_start.tv_usec;
        
        if (diff_time >= test_time)
        {
            break;
        }

        send_packet_one_epoch(payload, interval);
    }
#else
    long test_time = ((long)atoi(input + 5)) * 1000;  // ms
    clock_t t_start = clock();

    while (clock() - t_start < test_time)
    {
        send_packet_one_epoch(payload, interval);
    }
#endif  // !WIN32

    printf("Info: Successfully send %d packets!\n", g_sender.send_cnt);
}

static void sender_send_receive(void)
{
    char payload[MAX_PAYLOAD_LEN] = {'\0'};
    int interval = (g_config.interval > 0) ? g_config.interval :
                                             INTERVAL_SEND;

    while ((int)g_sender.send_cnt < g_config.num_pkts)
    {
        send_packet_one_epoch(payload, interval);
    }

    printf("Info: Successfully send %d packets!\n", g_sender.send_cnt);
}

#ifndef WIN32
void *sender_thread(void *param)
#else
unsigned int WINAPI sender_thread(void *param)
#endif  // !WIN32
{
    // Initialize the sender
    g_sender.send_cnt = 0;
    g_sender.seq = 0;

    // Start Interactive interface according to the mode selected
    if (MODE_SEND == g_config.mode)
    {
        char input[1000] = {'\0'};
        int c;
        int i = 0;

        // Print usage 
        printf("Info: Input command. For example:\n"
            "\t--- 'send 60': send 60 packets to the opposite\n"
            "\t--- 'test 60': keep sending packets in 60 seconds\n"
            "\t--- 'quit': quit this program\n");

        while (1)
        {
            printf("> ");
            while ((c = getchar()) != '\n' && c != EOF)
            {
                input[i++] = (char)c;
            }
            input[i] = '\0';

            // Parse command
            if (0 == strlen(input))  // single '\n'
            {
                printf("\n");
                continue;
            }

            if (0 == memcmp(input, "send", 4))
            {
                sender_send(input);
            }
            else if (0 == memcmp(input, "test ", 5))
            {
                sender_test(input);
            }
            else if (0 == memcmp(input, "quit", 4))
            {
                break;
            }
            else
            {
                printf("Error: Invalid command \"%s\"!\n", input);
            }

            g_sender.send_cnt = 0;
            i = 0;
        }
    }
    else if (MODE_SEND_RECEIVE == g_config.mode)  // Just send
    {
        sender_send_receive();
    }
    else
    {
        printf("[%s] Error: Cannot create sender thread under mode %d\n", \
            __FUNCTION__, g_config.mode);
        exit(EXIT_FAILURE);
    }

    printf("Info: Sender finishes its task and exits!\n");

#ifndef WIN32
    return NULL;
#else
    CloseHandle(g_sender.pid);
    return 0;
#endif  // !WIN32
}
