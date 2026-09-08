#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <net/if.h>        // для if_nametoindex
#include <netpacket/packet.h>  // для struct sockaddr_ll, struct packet_mreq, SOL_PACKET

#define BUFFER_SIZE 65536
#define MAX_PACKETS 1000
#define OUTPUT_FILE "captured_udp.txt"

volatile sig_atomic_t running = 1;

struct packet_info {
    double time_offset;
    unsigned char src_mac[6];
    unsigned char dst_mac[6];
    char src_ip[INET_ADDRSTRLEN];
    char dst_ip[INET_ADDRSTRLEN];
    unsigned short src_port;
    unsigned short dst_port;
    char protocol[16];
};

struct packet_info packets[MAX_PACKETS];
int packet_count = 0;
int filter_mode = 0;

void handle_signal(int sig)
{
    (void)sig;
    running = 0;
}

void print_mac(const unsigned char *mac, char *out)
{
    snprintf(out, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
             mac[0], mac[1], mac[2],
             mac[3], mac[4], mac[5]);
}

int filter_packet(unsigned short src_port, unsigned short dst_port)
{
    if (filter_mode == 0)
        return src_port == 8888 || dst_port == 8888;

    if (filter_mode == 1)
        return src_port == 53 || dst_port == 53;

    return 1;
}

int main(int argc, char *argv[])
{
    int duration = 30;

    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Использование: %s <chat|dns|all> [seconds]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "chat") == 0)
        filter_mode = 0;
    else if (strcmp(argv[1], "dns") == 0)
        filter_mode = 1;
    else if (strcmp(argv[1], "all") == 0)
        filter_mode = 2;
    else {
        fprintf(stderr, "Использование: %s <chat|dns|all> [seconds]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (argc == 3) {
        duration = atoi(argv[2]);
        if (duration <= 0) {
            fprintf(stderr, "Некорректная длительность\n");
            return EXIT_FAILURE;
        }
    }

    if (geteuid() != 0) {
        fprintf(stderr, "Требуются права root.\n");
        return EXIT_FAILURE;
    }

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    int sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP));
    if (sock < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

        int ifindex = if_nametoindex("wlp2s0");
    if (ifindex == 0) {
        perror("if_nametoindex");
        close(sock);
        return EXIT_FAILURE;
    }

    struct sockaddr_ll sll;
    memset(&sll, 0, sizeof(sll));
    sll.sll_family = AF_PACKET;
    sll.sll_protocol = htons(ETH_P_IP);
    sll.sll_ifindex = ifindex;

    if (bind(sock, (struct sockaddr *)&sll, sizeof(sll)) < 0) {
        perror("bind");
        close(sock);
        return EXIT_FAILURE;
    }

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt");
        close(sock);
        return EXIT_FAILURE;
    }

    unsigned char buffer[BUFFER_SIZE];
    struct timeval start, now;

    gettimeofday(&start, NULL);

    printf("Захват UDP-сегментов (%s) в течение %d секунд...\n",
           filter_mode == 0 ? "чат порт 8888" :
           filter_mode == 1 ? "DNS порт 53" : "все UDP", duration);
    printf("Нажмите Ctrl+C для досрочного завершения.\n");

    while (running) {
        gettimeofday(&now, NULL);

        double elapsed =
            (double)(now.tv_sec - start.tv_sec) +
            (double)(now.tv_usec - start.tv_usec) / 1000000.0;

        if (elapsed >= duration)
            break;

        ssize_t len = recvfrom(sock, buffer, sizeof(buffer), 0, NULL, NULL);

        if (len < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
                continue;

            perror("recvfrom");
            break;
        }

        if (len < (ssize_t)sizeof(struct ether_header))
            continue;

        struct ether_header *eth =
            (struct ether_header *)buffer;

        if (ntohs(eth->ether_type) != ETHERTYPE_IP)
            continue;

        if (len < (ssize_t)(sizeof(struct ether_header) +
                            sizeof(struct iphdr)))
            continue;

        struct iphdr *ip =
            (struct iphdr *)(buffer + sizeof(struct ether_header));

        if (ip->protocol != IPPROTO_UDP)
            continue;

        int ip_header_len = ip->ihl * 4;

        if (ip_header_len < 20 ||
            len < (ssize_t)(sizeof(struct ether_header) + ip_header_len +
                            sizeof(struct udphdr)))
            continue;

        struct udphdr *udp =
            (struct udphdr *)((unsigned char *)ip + ip_header_len);

        unsigned short src_port = ntohs(udp->source);
        unsigned short dst_port = ntohs(udp->dest);

        if (!filter_packet(src_port, dst_port))
            continue;

        if (packet_count >= MAX_PACKETS)
            break;

        struct packet_info *pkt = &packets[packet_count];

        gettimeofday(&now, NULL);
        pkt->time_offset =
            (double)(now.tv_sec - start.tv_sec) +
            (double)(now.tv_usec - start.tv_usec) / 1000000.0;

        memcpy(pkt->src_mac, eth->ether_shost, 6);
        memcpy(pkt->dst_mac, eth->ether_dhost, 6);

        inet_ntop(AF_INET, &ip->saddr,
                  pkt->src_ip, sizeof(pkt->src_ip));
        inet_ntop(AF_INET, &ip->daddr,
                  pkt->dst_ip, sizeof(pkt->dst_ip));

        pkt->src_port = src_port;
        pkt->dst_port = dst_port;

        if (src_port == 8888 || dst_port == 8888)
            strcpy(pkt->protocol, "CHAT");
        else if (src_port == 53 || dst_port == 53)
            strcpy(pkt->protocol, "DNS");
        else
            strcpy(pkt->protocol, "OTHER");

        packet_count++;
    }

    close(sock);

    printf("\nЗахвачено пакетов: %d\n", packet_count);

    FILE *fp = fopen(OUTPUT_FILE, "w");
    if (fp == NULL) {
        perror("fopen");
        return EXIT_FAILURE;
    }

    fprintf(fp, "Захваченные UDP-сегменты (фильтр: %s)\n",
            filter_mode == 0 ? "чат (порт 8888)" :
            filter_mode == 1 ? "DNS (порт 53)" : "все UDP");

    fprintf(fp,
            "Время от начала | Src MAC           | Dst MAC           | "
            "Src IP          | Dst IP          | Src Port | Dst Port | Protocol\n");

    fprintf(fp,
            "---------------------------------------------------------------------------------------------\n");

    for (int i = 0; i < packet_count; i++) {
        char src_mac[18];
        char dst_mac[18];

        print_mac(packets[i].src_mac, src_mac);
        print_mac(packets[i].dst_mac, dst_mac);

        char line[512];

        snprintf(line, sizeof(line),
                 "%9.6f | %17s | %17s | %15s | %15s | "
                 "%8d | %8d | %s\n",
                 packets[i].time_offset,
                 src_mac,
                 dst_mac,
                 packets[i].src_ip,
                 packets[i].dst_ip,
                 packets[i].src_port,
                 packets[i].dst_port,
                 packets[i].protocol);

        printf("%s", line);
        fprintf(fp, "%s", line);
    }

    fclose(fp);

    printf("\nРезультаты сохранены в файл %s\n", OUTPUT_FILE);

    return 0;
}