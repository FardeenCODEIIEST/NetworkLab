/* From stackoverflow, used in creation of recv and sendv handler threads*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 9666
#define BUFFER_SIZE 1024
#define ID_SIZE 50
#define MESSAGE_SIZE (BUFFER_SIZE - ID_SIZE - 1)

int sock;
pthread_t receive_thread;
char target_id[ID_SIZE];

void *receive_handler(void *arg)
{
    char buffer[BUFFER_SIZE];
    int read_size;

    while ((read_size = recv(sock, buffer, BUFFER_SIZE, 0)) > 0)
    {
        buffer[read_size] = '\0';
        printf("\nReceived: %s\n", buffer);
    }

    return 0;
}

int main(int argc, char *argv[])
{
    struct sockaddr_in server;
    char message[MESSAGE_SIZE], id[ID_SIZE];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        printf("Could not create socket");
    }

    server.sin_addr.s_addr = inet_addr(SERVER_IP);
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("Connect failed. Error");
        return 1;
    }

    printf("Enter your ID: ");
    fgets(id, ID_SIZE, stdin);
    id[strcspn(id, "\n")] = 0;

    send(sock, id, strlen(id), 0);

    printf("Enter target ID: ");
    fgets(target_id, ID_SIZE, stdin);
    target_id[strcspn(target_id, "\n")] = 0;

    pthread_create(&receive_thread, NULL, receive_handler, NULL);

    while (1)
    {
        printf("\nEnter message: ");
        fgets(message, MESSAGE_SIZE, stdin);
        message[strcspn(message, "\n")] = 0;

        char full_message[BUFFER_SIZE];
        snprintf(full_message, BUFFER_SIZE, "%s %s", target_id, message);

        send(sock, full_message, strlen(full_message), 0);
    }

    close(sock);
    return 0;
}
