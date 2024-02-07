#include <stdio.h>      // for printf(),..
#include <stdlib.h>     // for atoi(),...
#include <string.h>     // for memset(),memcpy(),..
#include <sys/socket.h> // for socket(),bind(),..
#include <arpa/inet.h>  // for inet_addr(),htons,...
#include <unistd.h>     // for close(),..
#include <sys/time.h>   // For gettimeofday()

#define MAX_BUFFER_SIZE 1007

char const_buffer[1007];

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
    double averageRTT = 0;     // Average RTT

    if (argc < 6)
    {
        fprintf(stderr, "Usage: %s <ServerIP> <ServerPort> <P> <TTL> <NumPackets>", argv[0]);
        exit(EXIT_SUCCESS);
    }
    // Check Command-line arguments
    if(atoi(argv[3])<100||atoi(argv[3])>1000||atoi(argv[4])<2||atoi(argv[4])>20||atoi(argv[4])%2!=0||atoi(argv[5])<1||atoi(argv[5])>50){
    	fprintf(stderr,"Invalid inputs, 2<=TTL<=20 and TTL is even and 100<=P<=1000 and 1<=numPackets<=50, How dare you change the assignment statement\n");
	exit(EXIT_FAILURE);
    }
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

    int numPackets = atoi(argv[5]);
    for (int i = 0; i < numPackets; i++)
    {
        // Prepare the packet (packet fields should be properly initialized)
        packet.TTL = atoi(argv[4]);
        packet.sequenceNumber = i;
        packet.payloadLength = atoi(argv[3]);
        // padding the packet with -1
        packet.payloadBytes = (char *)malloc(atoi(argv[3]) * sizeof(char));
        memcpy(packet.payloadBytes, const_buffer, packet.payloadLength);

        // serialise the packet to be sent to the buffer
        serialize_packet(&packet, buffer);

        // initial time
        gettimeofday(&start, NULL);

        // printf("Payload sent : %s\n", packet.payloadBytes);
        // send the packet to the server
        int c = sendto(sockfd, buffer, 7 + packet.payloadLength, 0, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
        if (c == -1)
        {
            perror("sendto() failed\n");
            exit(EXIT_FAILURE);
        }
        addr_size = sizeof(serverAddr);
        // receive the packet from the server
        int n = recvfrom(sockfd, buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr *)&serverAddr, &addr_size);

        gettimeofday(&end, NULL); // End time

        if (n > 0)
        {
            // Packet is correctly echoed back
            deserialize_packet(buffer, &packet);
            if (packet.TTL != atoi(argv[4]))
            {
                // packet is good
                // printf("Payload received : %s\n", packet.payloadBytes);
                // RTT
                long rtt = calculateRTT(start, end);
                averageRTT += rtt;
                printf("For packet with sequenceNumber:%d , RTT: %ld microseconds\n", packet.sequenceNumber, rtt);
            }
            else
            {
                // packet is not good
                printf("%s\n", packet.payloadBytes);
            }
        }
        else
        {
            perror("recvfrom() failed");
            exit(EXIT_FAILURE);
        }
        // free the payload
        free(packet.payloadBytes);
    }
    printf("Average RTT across all packets is: %lf microseconds\n", averageRTT / numPackets);
    close(sockfd);
    return 0;
}

