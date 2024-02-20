#include <stdio.h>      // for printf(),..
#include <stdlib.h>     // for atoi(),...
#include <string.h>     // for memset(),memcpy(),..
#include <sys/socket.h> // for socket(),bind(),..
#include <arpa/inet.h>  // for inet_addr(),htons,...
#include <unistd.h>     // for close(),..

#define MAX_BUFFER_SIZE 1007

char packet_dropped_Length[33] = "MALFORMED PACKET - Invalid Length";
char packet_invalid_TTL[30]    = "MALFORMED PACKET - Invalid TTL";

typedef struct
{
    uint8_t TTL;             // 1 byte TTL field denoting Time To Live
    uint32_t sequenceNumber; // 4 bytes sequenceNumber to uniquely identify a packet
    uint16_t payloadLength;  // 2 bytes payload length to indicate length of payload in bytes
    char *payloadBytes;      // payload of the packet
} Packet;

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
    int sockfd;                                // server socket descriptor
    struct sockaddr_in serverAddr, clientAddr; // server and client address
    uint8_t buffer[MAX_BUFFER_SIZE];           // 8 byte element buffer
    socklen_t clilen;                          // client address length
    Packet packet;                             // packet

    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <ServerIP> <ServerPort>\n", argv[0]);
        exit(EXIT_SUCCESS);
    }
    // For DATAGRAM sockets, we do not need listen() and bind() as said in the guide
    // socket(), bind(), do stuff (recvfrom(),sendto()), close()
    int port = atoi(argv[2]);

    // socket()
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    // bind
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(argv[1]);
    serverAddr.sin_port = htons(port);

    if (bind(sockfd, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", port);

    // do stuff
    while (1)
    {
        clilen = sizeof(clientAddr);
        int n = recvfrom(sockfd, (char *)buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr *)&clientAddr, &clilen);
        if (n < 0)
        {
            // skip this packet if failed
            perror("recvfrom() failed");
            continue;
        }

        // De-serialisation of buffer into a packet
        deserialize_packet(buffer, &packet);

        // Display packet info  ---> introduces a delay
        // display_info(&packet);

        // Sanity check: Received size - 7 == payloadLength  ---> introduces a delay
        // printf("Received Size:- %d, PayloadLength in packet is:- %d\n", n, packet.payloadLength);
        if ((packet.payloadLength == n - 7)&&(packet.TTL%2==0)) // tweak this to check for dropped packets
        {
            if (packet.TTL > 0)
            {
                packet.TTL--; // Decrement TTL
                // Serialisation of packet into a buffer for sending to the client
                serialize_packet(&packet, buffer);

                // send to the client
                sendto(sockfd, buffer, 7 + packet.payloadLength, 0, (struct sockaddr *)&clientAddr, clilen);
            }
        }
        else if(packet.TTL%2!=0)
        {
            printf("Packet sanity check failed.\n");

            memcpy(packet.payloadBytes, packet_invalid_TTL, 30);
            packet.payloadBytes[30] = '\0';
            serialize_packet(&packet, buffer);

            // sendto() packet drop message to the client
            sendto(sockfd, buffer, 7 + packet.payloadLength, 0, (struct sockaddr *)&clientAddr, clilen);

            // Skip this packet
            continue;
        }
	else{
	  
	    printf("Packet sanity check failed.\n");

            memcpy(packet.payloadBytes, packet_dropped_Length, 33);
            packet.payloadBytes[33] = '\0';
            serialize_packet(&packet, buffer);

            // sendto() packet drop message to the client
            sendto(sockfd, buffer, 7 + packet.payloadLength, 0, (struct sockaddr *)&clientAddr, clilen);

            // Skip this packet
            continue;

	}

        free(packet.payloadBytes); // free dynamically allocated payloadBytes
    }

    close(sockfd);
    return 0;
}
