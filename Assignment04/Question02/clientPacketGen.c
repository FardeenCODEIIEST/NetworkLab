#include <stdio.h>      // for printf(),..
#include <stdlib.h>     // for atoi(),...
#include <string.h>     // for memset(),memcpy(),..
#include <sys/socket.h> // for socket(),bind(),..
#include <arpa/inet.h>  // for inet_addr(),htons,...
#include <unistd.h>     // for close(),..
#include <sys/time.h>   // For gettimeofday()

#define MAX_BUFFER_SIZE 1024

char const_buffer[1024];

// Assuming the Packet structure and serialization/deserialization functions are defined here
typedef struct
{
    uint8_t TTL;             // 1 byte TTL field denoting Time To Live
    uint32_t sequenceNumber; // 4 bytes sequenceNumber to uniquely identify a packet
    uint16_t payloadLength;  // 2 bytes payload length to indicate length of payload in bytes
    char *payloadBytes;      // payload of the packet
} Packet;

/*
    @param:- (struct timeval: start_time,struct timeval: end_time)
    @return : long: time difference
    The function calculates the round trip time for a packet
*/
long calculateRTT(struct timeval start, struct timeval end)
{
    long seconds = end.tv_sec - start.tv_sec;
    long micro_seconds = end.tv_usec - start.tv_usec;
    return seconds * 1000000 + micro_seconds; // Convert to microseconds
}

/*
    @param:- (uint8_t* : buffer, struct Packet*: packet)
    @return : void
    The function deserializes a buffer and feeds the info into a packet
*/
void deserialize_packet(uint8_t *buffer, Packet *packet)
{
    uint32_t seqNum;
    memcpy(&seqNum, buffer, sizeof(seqNum));
    packet->sequenceNumber = ntohl(seqNum); // byte order is big-endian so make it little endian
    memcpy(&packet->TTL, buffer + 4, sizeof(packet->TTL));
    uint16_t payloadLen;
    memcpy(&payloadLen, buffer + 5, sizeof(payloadLen));
    packet->payloadLength = ntohs(payloadLen);                // byte order is big-endian so make it little endian
    packet->payloadBytes = malloc(packet->payloadLength + 1); // +1 to terminate the packet with '\0'
    memcpy(packet->payloadBytes, buffer + 7, packet->payloadLength);
    packet->payloadBytes[packet->payloadLength] = '\0'; // Null-termination
}

/*
    @param:- (uint8_t* : buffer, struct Packet*: packet)
    @return : void
    The function serializes a packet info into a buffer
*/
void serialize_packet(Packet *packet, uint8_t *buffer)
{
    uint32_t seqNum = htonl(packet->sequenceNumber); // byte order is little endian so convert to bing-endian
    memcpy(buffer, &seqNum, sizeof(seqNum));
    memcpy(buffer + 4, &packet->TTL, sizeof(packet->TTL));
    uint16_t payloadLen = htons(packet->payloadLength); // byte order is little endian so convert to bing-endian
    memcpy(buffer + 5, &payloadLen, sizeof(payloadLen));
    memcpy(buffer + 7, packet->payloadBytes, packet->payloadLength);
}

/*
    @param:- (struct Packet*: packet)
    @return : void
    The function displays packet information
*/

void display_info(Packet *packet)
{
    printf("The Sequence Number of the Packet is : %d\n", packet->sequenceNumber);
    printf("The TTL field of the Packet is : %d\n", packet->TTL);
    printf("The PayloadLength of the Packet is : %d\n", packet->payloadLength);
    printf("The Payload of the Packet is : %s\n", packet->payloadBytes);
}

int main(int argc, char *argv[])
{
    int sockfd;
    struct sockaddr_in serverAddr;
    socklen_t addr_size;
    uint8_t buffer[MAX_BUFFER_SIZE];
    Packet packet;
    struct timeval start, end; // For RTT calculation
    double averageRTT;         // Average RTT
    int cummRTT;               // Cumulative RTT
    if (argc < 6)
    {
        fprintf(stderr, "Usage: %s <ServerIP> <ServerPort> <P> <TTL> <FilePath>", argv[0]);
        exit(EXIT_SUCCESS);
    }
    // Setting up the file pointer
    FILE *fp = fopen(argv[5], "w");
    if (fp == NULL)
    {
        perror("File open error\n");
        exit(EXIT_FAILURE);
    }
    fprintf(fp, "%s,%s\n", "Value of payload(in bytes)", "Cumulative RTT(in microseconds)");
    // Set the buffer
    for (int i = 0; i < MAX_BUFFER_SIZE; i++)
    {
        const_buffer[i] = 'a'; // padding with 'a'
    }

    // socket(), do stuff (recvfrom(), sendto()), close() ---> no need to do connect()
    // socket()
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("socket failed:\n");
        exit(EXIT_FAILURE);
    }

    // server port and IP
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(atoi(argv[2]));
    serverAddr.sin_addr.s_addr = inet_addr(argv[1]);

    for (int payLen = atoi(argv[3]); payLen <= 1000; payLen += 100)
    {
        int numPackets = 50;
        printf("<-------------------- Packet size is %d -----------------------------> \n", payLen);
        averageRTT = 0;
        for (int i = 0; i < numPackets; i++)
        {
            // Prepare the packet (packet fields should be properly initialized)
            packet.TTL = atoi(argv[4]);
            packet.sequenceNumber = i;
            packet.payloadLength = payLen;
            // padding the packet with -1
            packet.payloadBytes = (char *)malloc(payLen * sizeof(char));
            memcpy(packet.payloadBytes, const_buffer, packet.payloadLength);

            // serialise the packet to be sent to the buffer
            serialize_packet(&packet, buffer);

            // initial time
            cummRTT = 0;
            gettimeofday(&start, NULL);

            // printf("Payload sent : %s\n", packet.payloadBytes);
            // send the packet to the server until TTL is not zero
            while (packet.TTL != 0) // send the packets until and unless the TTL is 0
            {
                int c = sendto(sockfd, buffer, 7 + packet.payloadLength, 0, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
                if (c == -1)
                {
                    perror("sendto() failed\n");
                    exit(EXIT_FAILURE);
                }
                addr_size = sizeof(serverAddr);
                // receive the packet from the server
                int n = recvfrom(sockfd, buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr *)&serverAddr, &addr_size);

                if (n > 0)
                {
                    // Packet is correctly echoed back
                    deserialize_packet(buffer, &packet);
                    if (packet.TTL == atoi(argv[4]))
                    {
                        // packet is not good
                        printf("%s\n", packet.payloadBytes);
                        exit(0);
                    }
                }
                else
                {
                    perror("recvfrom() failed");
                    exit(EXIT_FAILURE);
                }
            }
            gettimeofday(&end, NULL);
            cummRTT = calculateRTT(start, end);
            printf("For packet with sequence number %d, cumulative RTT is :%d microseconds\n", packet.sequenceNumber, cummRTT);
            averageRTT += cummRTT;
            cummRTT = 0;
            // free the payload
            free(packet.payloadBytes);
        }
        printf("Average RTT across all packets is: %lf microseconds\n", averageRTT / numPackets);
        fprintf(fp, "%d,%lf\n", payLen, averageRTT / numPackets);
    }
    printf("File has been saved successfully\n");
    close(sockfd);
    fclose(fp);
    return 0;
}
