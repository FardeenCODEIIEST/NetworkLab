#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>

#define SERVER_PORT 12345
#define SERVER_IP "127.0.0.1"
#define BUF_SIZE 1024
#define WINDOW_SIZE 4
#define MAX_PACKETS 10
#define RETRANSMIT_TIMEOUT 5 // Timeout in seconds for retransmission

int sockfd;
struct sockaddr_in servaddr;
socklen_t len;
volatile int ackedSeqNum = 0; // Tracks the highest sequence number acknowledged
volatile int windowBase = 1; // Tracks the base of the current window
char window[WINDOW_SIZE][BUF_SIZE]; // Buffer to store sent packets for potential retransmission
volatile int packetsToSend = MAX_PACKETS; // Total packets to send for the demonstration
pthread_mutex_t lock;

void* ack_listener(void *arg) {
    char buffer[BUF_SIZE];
    while (1) {
        int n = recvfrom(sockfd, buffer, BUF_SIZE, 0, NULL, NULL);
        if (n > 0) {
            buffer[n] = '\0';
            int seqNum;
            if (sscanf(buffer, "ACK %d", &seqNum) == 1) {
                printf("ACK received for sequence number: %d\n", seqNum);
                pthread_mutex_lock(&lock);
                ackedSeqNum = seqNum;
                windowBase = ackedSeqNum + 1;
                pthread_mutex_unlock(&lock);
            }
        }
    }
    return NULL;
}

void retransmit_window() {
    printf("Retransmitting window...\n");
    pthread_mutex_lock(&lock);
    for (int i = 0; i < WINDOW_SIZE && (windowBase + i) <= packetsToSend; i++) {
        printf("Retransmitting packet with sequence number: %d\n", windowBase + i);
        sendto(sockfd, window[i], strlen(window[i]), 0, (const struct sockaddr *)&servaddr, len);
    }
    pthread_mutex_unlock(&lock);
}

int main() {
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

    pthread_mutex_init(&lock, NULL);
    pthread_t tid;
    pthread_create(&tid, NULL, ack_listener, NULL); // Start ACK listener thread

    time_t lastSentTime = time(NULL); // Track the last time packets were sent

    while (windowBase <= packetsToSend) {
        time_t currentTime = time(NULL);
        if (currentTime - lastSentTime >= RETRANSMIT_TIMEOUT) {
            retransmit_window();
            lastSentTime = currentTime;
        }

        // Send new packets if there is space in the window and packets left to send
        pthread_mutex_lock(&lock);
        while (windowBase + WINDOW_SIZE > ackedSeqNum && (windowBase <= packetsToSend)) {
            int windowIndex = (windowBase - ackedSeqNum - 1) % WINDOW_SIZE;
            sprintf(window[windowIndex], "%d: Packet", windowBase);
            sendto(sockfd, window[windowIndex], strlen(window[windowIndex]), 0, (const struct sockaddr *)&servaddr, len);
            printf("Sent packet with sequence number: %d\n", windowBase);
            windowBase++;
            lastSentTime = time(NULL);
        }
        pthread_mutex_unlock(&lock);
        sleep(1); // Avoid tight loop, replace with more sophisticated timing in real applications
    }

    pthread_join(tid, NULL); // Wait for ACK listener thread to finish
    pthread_mutex_destroy(&lock);
    close(sockfd);
    return 0;
}

