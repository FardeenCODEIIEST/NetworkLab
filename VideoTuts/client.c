#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define SERVER_PORT 12345
#define SERVER_IP "127.0.0.1"
#define BUF_SIZE 1024
#define TIMEOUT_SEC 2 // Timeout in seconds

int sockfd;
struct sockaddr_in servaddr;
socklen_t len;
volatile int ack_received = 0; // Use volatile for variables accessed by multiple threads

void* send_message(void *arg) {
    char *message = (char *)arg;
    sendto(sockfd, message, strlen(message), MSG_CONFIRM, (const struct sockaddr *) &servaddr, len);
    printf("Message sent.\n");
    return NULL;
}

void* receive_ack(void *arg) {
    char buffer[BUF_SIZE];
    int n;
    n = recvfrom(sockfd, (char *)buffer, BUF_SIZE, MSG_WAITALL, (struct sockaddr *) &servaddr, &len);
    if (n > 0) {
        buffer[n] = '\0';
        printf("Server: %s\n", buffer);
        ack_received = 1; // ACK received
    }
    return NULL;
}

void* timeout_thread(void *arg) {
    int timeout = *(int *)arg;
    sleep(timeout);
    if (!ack_received) {
        printf("Timeout occurred. No ACK received.\n");
    }
    return NULL;
}

int main() {
    pthread_t send_thread, recv_thread, timer_thread;
    int timeout = TIMEOUT_SEC;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVER_PORT);
    servaddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    len = sizeof(servaddr);

    char message[] = "Hello, Server!";
    if (pthread_create(&send_thread, NULL, send_message, (void *)message)) {
        perror("Failed to create send thread");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&recv_thread, NULL, receive_ack, NULL)) {
        perror("Failed to create receive thread");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&timer_thread, NULL, timeout_thread, (void *)&timeout)) {
        perror("Failed to create timer thread");
        exit(EXIT_FAILURE);
    }

    pthread_join(send_thread, NULL);
    pthread_join(recv_thread, NULL);
    pthread_join(timer_thread, NULL);

    close(sockfd);
    return 0;
}

