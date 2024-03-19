#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <net/if.h>
#include <netpacket/packet.h>
#include <pthread.h>
#include <stdbool.h>

#define ARP_PROTOCOL 0x0806
#define SENDER_IP "192.168.29.41"

char SMAC[6] = {0x48, 0xE7, 0XDA, 0XA7, 0X7B, 0XA5};
char DMAC[6] = {0xFF, 0XFF, 0XFF, 0XFF, 0XFF, 0XFF};
char TMAC[6] = {0x00, 0X00, 0X00, 0X00, 0X00, 0X00};

volatile int replyReceived = 0;

typedef struct
{
    // L2 Specification
    char DMAC[6];     // Destination MAC Address
    char SMAC[6];     // Source MAC Address
    short ether_type; // Protocol type of above layer
    // Below can be ignored
    // char padding[10];
    // char CRC[4];
    //---------------------------------------------------------------
    // L3 Specification
    short htype;        // hardware type
    short ptype;        // protocol type
    uint8_t hlen;       // hardware address length
    uint8_t plen;       // protocol address length
    short opcode;       // Opcode
    char sender_hw[6];  // sender hardware address
    uint32_t sender_ip; // sender ip address
    char target_hw[6];  // target hardware address
    uint32_t target_ip; // target ip address

} arpPacket;

void *timer_function(void *timeout)
{
    int t = *(int *)timeout;
    sleep(t); // Wait for 't' seconds
    if (!replyReceived)
    {
        printf("Did not receive any reply\n");
        replyReceived = -1;
    }
    pthread_exit(NULL);
}

void serialise(arpPacket *packet, char *buffer)
{
    int index;

    // L2
    for (index = 0; index < 6; index++)
    {
        buffer[index] = packet->DMAC[index];
    }

    for (index = 6; index < 12; index++)
    {
        buffer[index] = packet->SMAC[index - 6];
    }

    buffer[index++] = packet->ether_type >> 8;
    buffer[index++] = packet->ether_type;

    // L3
    buffer[index++] = packet->htype >> 8;
    buffer[index++] = packet->htype;

    buffer[index++] = packet->ptype >> 8;
    buffer[index++] = packet->ptype;

    buffer[index++] = packet->hlen;

    buffer[index++] = packet->plen;

    buffer[index++] = packet->opcode >> 8;
    buffer[index++] = packet->opcode;

    for (int k = 0; k < 6; k++)
    {
        buffer[index + k] = packet->sender_hw[k];
    }
    index += 6;

    buffer[index++] = (packet->sender_ip >> 24) & 0xFF;
    buffer[index++] = (packet->sender_ip >> 16) & 0xFF;
    buffer[index++] = (packet->sender_ip >> 8) & 0xFF;
    buffer[index++] = (packet->sender_ip) & 0xFF;

    for (int k = 0; k < 6; k++)
    {
        buffer[index + k] = packet->target_hw[k];
    }
    index += 6;

    buffer[index++] = (packet->target_ip >> 24) & 0xFF;
    buffer[index++] = (packet->target_ip >> 16) & 0xFF;
    buffer[index++] = (packet->target_ip >> 8) & 0xFF;
    buffer[index++] = (packet->target_ip) & 0xFF;
}

void deserialise(char *buffer, arpPacket *packet)
{
    int index;

    // L2
    for (index = 0; index < 6; index++)
    {
        packet->DMAC[index] = buffer[index];
    }
    for (index = 6; index < 12; index++)
    {
        packet->SMAC[index - 6] = buffer[index];
    }
    uint8_t msb = (uint8_t)buffer[index++];
    uint8_t lsb = (uint8_t)buffer[index++];

    packet->ether_type = (msb << 8 | lsb);

    // L3

    msb = (uint8_t)buffer[index++];
    lsb = (uint8_t)buffer[index++];
    packet->htype = (msb << 8 | lsb);

    msb = (uint8_t)buffer[index++];
    lsb = (uint8_t)buffer[index++];
    packet->ptype = (msb << 8 | lsb);

    packet->hlen = (uint8_t)buffer[index++];

    packet->plen = (uint8_t)buffer[index++];

    msb = (uint8_t)buffer[index++];
    lsb = (uint8_t)buffer[index++];
    packet->opcode = (msb << 8 | lsb);

    for (int k = 0; k < 6; k++)
    {
        packet->sender_hw[k] = buffer[index + k];
    }

    index += 6;

    uint8_t p1, p2, p3, p4;
    p1 = (uint8_t)buffer[index++];
    p2 = (uint8_t)buffer[index++];
    p3 = (uint8_t)buffer[index++];
    p4 = (uint8_t)buffer[index++];

    packet->sender_ip = (p1 << 24 | p2 << 16 | p3 << 8 | p4);

    for (int k = 0; k < 6; k++)
    {
        packet->target_hw[k] = buffer[index + k];
    }

    index += 6;

    p1 = (uint8_t)buffer[index++];
    p2 = (uint8_t)buffer[index++];
    p3 = (uint8_t)buffer[index++];
    p4 = (uint8_t)buffer[index++];

    packet->target_ip = (p1 << 24 | p2 << 16 | p3 << 8 | p4);
}

// MAC validation
bool validate(char MAC[6])
{
    for (int i = 0; i < 6; i++)
    {
        if (MAC[i] != SMAC[i])
            return false;
    }
    return true;
}

int main(int argc, char *argv[])
{
    // Usage:- [exe_name] [interface name] [dest_ip]

    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <interface name> <destination_ip>\n", argv[0]);
        return 0;
    }

    struct sockaddr_ll src_addr, dest_addr;
    int sockRaw;

    // Raw Socket
    sockRaw = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sockRaw == -1)
    {
        perror("socket error:\n");
        return 0;
    }

    // Source MAC
    memset(&src_addr, 0, sizeof(struct sockaddr_ll));
    src_addr.sll_family = AF_PACKET;
    src_addr.sll_protocol = IPPROTO_RAW;
    src_addr.sll_ifindex = if_nametoindex(argv[1]);
    src_addr.sll_halen = ETH_ALEN;
    memcpy(src_addr.sll_addr, SMAC, 6);

    // Destination MAC
    memset(&dest_addr, 0, sizeof(struct sockaddr_ll));
    dest_addr.sll_family = AF_PACKET;
    dest_addr.sll_protocol = IPPROTO_RAW;
    dest_addr.sll_ifindex = if_nametoindex(argv[1]);
    dest_addr.sll_halen = ETH_ALEN;
    memcpy(dest_addr.sll_addr, DMAC, 6);

    arpPacket *packet = (arpPacket *)malloc(sizeof(arpPacket));
    // Prepare Packet
    for (int i = 0; i < 6; i++)
    {
        packet->SMAC[i] = SMAC[i];
        packet->sender_hw[i] = SMAC[i];
        packet->DMAC[i] = DMAC[i];
        packet->target_hw[i] = TMAC[i];
    }

    packet->ether_type = ARP_PROTOCOL;

    packet->htype = 1;
    packet->ptype = 0x800;
    packet->hlen = 6;
    packet->plen = 4;

    packet->opcode = 1; // request

    // int dest_ip = inet_addr(ip);
    packet->target_ip = htonl(inet_addr(argv[2]));
    packet->sender_ip = htonl(inet_addr(SENDER_IP));

    // serialise the packet
    char *buffer = (char *)malloc(sizeof(char) * 42);
    serialise(packet, buffer);

    // Before sending the ARP request
    pthread_t timer_thread;
    int timeout_time = 5;
    if (pthread_create(&timer_thread, NULL, timer_function, &timeout_time) != 0)
    {
        perror("Failed to create timer thread");
        close(sockRaw);
        return 0;
    }

    // Send the buffer
    if (sendto(sockRaw, buffer, 42, 0, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_ll)) == -1)
    {
        perror("sento() failed\n");
        close(sockRaw);
        return 0;
    }
    free(buffer);
    int recv_len;
    char *recvBuffer = (char *)malloc(sizeof(char) * 42);
    // Receive the buffer
    while (!replyReceived)
    {
        recv_len = recv(sockRaw, recvBuffer, 42, 0);
        if (recv_len == 42)
        {
            // deserialise
            deserialise(recvBuffer, packet);
            short opcode = ntohs(packet->opcode);
            // validation check
            if (opcode >> 8 == 2 && validate(packet->DMAC))
            {
                replyReceived = 1;
                printf("ARP reply received\n");
                printf("DMAC is :\n");
                for (int k = 0; k < 6; k++)
                {
                    if (k == 5)
                        printf("%02X\n", (uint8_t)packet->SMAC[k]);
                    else
                        printf("%02X:", (uint8_t)packet->SMAC[k]);
                }
                free(recvBuffer);
                free(packet);
                close(sockRaw);

                exit(EXIT_SUCCESS);
            }
        }
    }
    // wait for the thread to terminate
    pthread_join(timer_thread, NULL);
    free(packet);
    free(recvBuffer);
    close(sockRaw);
    return 0;
}
