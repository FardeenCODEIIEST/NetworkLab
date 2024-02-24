#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
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
    int n;

    // Create a UDP Socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // Fill server information
    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(SERVER_PORT);

    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", SERVER_PORT);

    len = sizeof(cliaddr);  //len is value/resuslt

    while (1) {
        n = recvfrom(sockfd, (char *)buffer, BUF_SIZE,MSG_WAITALL ,(struct sockaddr *) &cliaddr, &len);
        buffer[n] = '\0';
        printf("Received: %s\n", buffer);

        // Send ACK back to the client
        sendto(sockfd, "ACK", strlen("ACK"), MSG_CONFIRM,(const struct sockaddr *) &cliaddr, len);
        printf("ACK sent.\n");
    }

    return 0;
}

