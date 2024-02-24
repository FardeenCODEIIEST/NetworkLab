#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_PORT 12345
#define BUF_SIZE 1024

int main() {
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    char buffer[BUF_SIZE];
    socklen_t len;
    int expectedSeqNum = 1;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(SERVER_PORT);

    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    printf("Server is running on port %d\n", SERVER_PORT);
    len = sizeof(cliaddr);

    while (1) {
        int n = recvfrom(sockfd, buffer, BUF_SIZE, 0, (struct sockaddr *)&cliaddr, &len);
        if (n < 0) {
            break; // Exit if error
        }
        buffer[n] = '\0';

        int seqNum;
        sscanf(buffer, "%d", &seqNum); // Assuming the message starts with a sequence number

        printf("Received packet with sequence number: %d\n", seqNum);

        if (seqNum == expectedSeqNum) {
            printf("ACK sent for sequence number: %d\n", seqNum);
            char ackMsg[20];
            sprintf(ackMsg, "ACK %d", seqNum);
            sendto(sockfd, ackMsg, strlen(ackMsg), 0, (const struct sockaddr *)&cliaddr, len);
            expectedSeqNum++; // Expect the next in-sequence packet
        }
    }

    close(sockfd);
    return 0;
}

