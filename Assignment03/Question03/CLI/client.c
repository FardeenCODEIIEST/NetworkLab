#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_USERNAME_LEN 10
#define MAX_MESSAGE_LEN 256

int client_socket;

void *receive_messages(void *arg)
{
    char buffer[MAX_MESSAGE_LEN];

    while (1)
    {
        ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0)
        {
            printf("Server disconnected. Exiting...\n");
            exit(EXIT_FAILURE);
        }

        buffer[bytes_received] = '\0';

        printf("\n%s\n", buffer);
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s server_ip port_no\n", argv[0]);
        exit(1);
    }
    struct sockaddr_in server_addr;

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));

    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0)
    {
        perror("Invalid address");
        exit(EXIT_FAILURE);
    }

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    char username[MAX_USERNAME_LEN];
    printf("Enter your username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = '\0';

    send(client_socket, username, strlen(username), 0);

    char server_response[MAX_MESSAGE_LEN];
    recv(client_socket, server_response, sizeof(server_response), 0);
    printf("%s\n", server_response);

    pthread_t receive_thread;
    pthread_create(&receive_thread, NULL, receive_messages, NULL);

    // Request undelivered messages from the server
    send(client_socket, "request_messages", strlen("request_messages"), 0);

    char recipient[MAX_USERNAME_LEN];
    printf("Enter recipient's username (or type 'exit' to close the chat): ");
    fgets(recipient, sizeof(recipient), stdin);
    recipient[strcspn(recipient, "\n")] = '\0';

    printf("Welcome, %s! Start chatting...\n", username);

    char message[MAX_MESSAGE_LEN];
    while (1)
    {
        printf("Enter message to %s: ", recipient);
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';

        char combined_message[MAX_MESSAGE_LEN + MAX_USERNAME_LEN + 1];
        snprintf(combined_message, sizeof(combined_message), "%s %s", recipient, message);
        send(client_socket, combined_message, strlen(combined_message), 0);

        if (strcmp(message, "exit") == 0)
        {
            printf("You requested to close the chat. Exiting...\n");
            break;
        }

        if (strcmp(message, "exit") == 0)
        {
            break;
        }
    }

    close(client_socket);

    return 0;
}