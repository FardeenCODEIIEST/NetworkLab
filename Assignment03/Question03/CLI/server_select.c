#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/select.h>

#define MAX_CLIENTS 10
#define MAX_USERNAME_LEN 10
#define MAX_MESSAGE_LEN 256
#define COMMAND_LENGTH 20

typedef struct
{
    int client_socket;
    char username[MAX_USERNAME_LEN];
    int status;
} Client;

typedef struct
{
    char sender[MAX_USERNAME_LEN];
    char recipient[MAX_USERNAME_LEN];
    char message[MAX_MESSAGE_LEN];
    int delivered_status;
} ChatMessage;

Client clients[MAX_CLIENTS];
pthread_t client_threads[MAX_CLIENTS];
pthread_t server_CLI;
int client_count = 0;
int server_socket, client_socket;

fd_set set;
int max_fd;

ChatMessage chat_messages[MAX_CLIENTS * MAX_CLIENTS];
int chat_message_count = 0;

void save_message(const char *sender, const char *recipient, const char *message, int delivered_status)
{

    strncpy(chat_messages[chat_message_count].sender, sender, MAX_USERNAME_LEN - 1);
    chat_messages[chat_message_count].sender[MAX_USERNAME_LEN - 1] = '\0';

    strncpy(chat_messages[chat_message_count].recipient, recipient, MAX_USERNAME_LEN - 1);
    chat_messages[chat_message_count].recipient[MAX_USERNAME_LEN - 1] = '\0';

    strncpy(chat_messages[chat_message_count].message, message, MAX_MESSAGE_LEN - 1);
    chat_messages[chat_message_count].message[MAX_MESSAGE_LEN - 1] = '\0';

    chat_messages[chat_message_count].delivered_status = delivered_status;

    chat_message_count++;
}

void deliver_messages(const char *username)
{

    for (int i = 0; i < chat_message_count; i++)
    {
        if (strcmp(chat_messages[i].recipient, username) == 0 && chat_messages[i].delivered_status == 0)
        {
            for (int j = 0; j < MAX_CLIENTS; j++)
            {
                if (clients[j].client_socket != -1 && strcmp(clients[j].username, username) == 0)
                {
                    char buffer[MAX_MESSAGE_LEN];
                    snprintf(buffer, sizeof(buffer), "\nFrom %s: %s", chat_messages[i].sender, chat_messages[i].message);
                    send(clients[j].client_socket, buffer, strlen(buffer), 0);
                    chat_messages[i].delivered_status = 1;
                    break;
                }
            }
        }
    }
}

void view_chat_filer(const char *filter_username)
{

    printf("Sender\tRecipient\tMessage\t\tDelivered Status\n");
    for (int i = 0; i < chat_message_count; i++)
    {
        if (strcmp(chat_messages[i].sender, filter_username) == 0 ||
            strcmp(chat_messages[i].recipient, filter_username) == 0 ||
            (strcmp(chat_messages[i].sender, filter_username) == 0 && strcmp(chat_messages[i].recipient, filter_username) == 0))
        {
            printf("%s\t%s\t\t%s\t(%s)\n", chat_messages[i].sender, chat_messages[i].recipient, chat_messages[i].message,
                   (chat_messages[i].delivered_status == 1) ? "Delivered" : "Not Delivered");
        }
    }
}

void view_chat_sender(const char *filter_username)
{

    printf("Your sent chat logs are :\n");
    printf("Recipient\tMessage\t\tDelivered Status\n");
    for (int i = 0; i < chat_message_count; i++)
    {
        if (strcmp(chat_messages[i].sender, filter_username) == 0)
        {
            printf("%s\t\t%s\t(%s)\n", chat_messages[i].recipient, chat_messages[i].message,
                   (chat_messages[i].delivered_status == 1) ? "Delivered" : "Not Delivered");
        }
    }
}

void view_chat_receiver(const char *filter_username)
{

    printf("Your received chat logs are :\n");
    printf("Sender\tMessage\t\tDelivered Status\n");
    for (int i = 0; i < chat_message_count; i++)
    {
        if (strcmp(chat_messages[i].recipient, filter_username) == 0)
        {
            printf("%s\t\t%s\t(%s)\n", chat_messages[i].sender, chat_messages[i].message,
                   (chat_messages[i].delivered_status == 1) ? "Delivered" : "Not Delivered");
        }
    }
}

void view_chat()
{

    printf("Sender\tRecipient\tMessage\n");
    for (int i = 0; i < chat_message_count; i++)
    {

        printf("%s\t%s\t\t%s\t(%s)\n", chat_messages[i].sender, chat_messages[i].recipient, chat_messages[i].message,
               (chat_messages[i].delivered_status == 1) ? "Delivered" : "Not Delivered");
    }
}

void show_users()
{
    printf("User_Name\tConnectionStatus\n");
    for (int i = 0; i < client_count; i++)
    {
        printf("%s\t(%s)\n", clients[i].username, (clients[i].status == 1) ? "Online" : ((clients[i].status == 0) ? "Offline" : "Banned"));
    }
}

void ban_user(const char *filter_username)
{
    int cli_index = -1;
    pthread_t cli_thread;
    for (int i = 0; i < client_count; i++)
    {
        if (strcmp(filter_username, clients[i].username) == 0)
        {
            cli_thread = client_threads[i];
            cli_index = i;
            break;
        }
    }
    close(clients[cli_index].client_socket);
    pthread_cancel(cli_thread);
    clients[cli_index].status = -1;
}

void *cli_handler(void *args)
{
    printf("Welcome to the server\n");
    while (1)
    {
        printf("Type :\n 1. See all chats\n 2. See chats with filter\n 3. Filter through Sender\n 4. Filter through Receiver\n 5. View all users\n 6. Ban User\n");
        int a;
        char user_name[MAX_USERNAME_LEN];
        bzero(user_name, MAX_USERNAME_LEN);
        scanf("%d", &a);
        switch (a)
        {
        case 1:
            view_chat();
            break;
        case 2:
            printf("Enter username: ");
            scanf("%s", user_name);
            view_chat_filer(user_name);
            break;
        case 3:
            printf("Enter Sender Username: ");
            scanf("%s", user_name);
            view_chat_sender(user_name);
            break;
        case 4:
            printf("Enter Receiver Username: ");
            scanf("%s", user_name);
            view_chat_receiver(user_name);
            break;
        case 5:
            printf("The users are:\n");
            show_users();
            break;
        case 6:
            printf("Enter the Username: ");
            scanf("%s", user_name);
            ban_user(user_name);
            break;
        default:
            printf("Try again:(\n");
        }
    }
    return NULL;
}

void *handle_client(void *arg)
{
    Client *client = (Client *)arg;
    char buffer[MAX_MESSAGE_LEN];

    snprintf(buffer, sizeof(buffer), "You are connected as: %s\n", client->username);
    send(client->client_socket, buffer, strlen(buffer), 0);

    deliver_messages(client->username);

    while (1)
    {
        ssize_t bytes_received = recv(client->client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0)
        {
            printf("Client '%s' disconnected.\n", client->username);
            for (int i = 0; i < client_count; i++)
            {
                if (strcmp(client->username, clients[i].username) == 0)
                {
                    clients[i].status = 0;
                    break;
                }
            }
            close(client->client_socket);
            break;
        }

        buffer[bytes_received] = '\0';

        char recipient[MAX_USERNAME_LEN];
        char message[MAX_MESSAGE_LEN];
        if (sscanf(buffer, "%s %[^\n]", recipient, message) == 2)
        {
            if (strcmp(recipient, "exit") == 0 && strcmp(message, "exit") == 0)
            {
                printf("Client '%s' requested to close the chat. Closing...\n", client->username);
                close(client->client_socket);
                break;
            }

            int recipient_index = -1;
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (clients[i].client_socket != -1 && strcmp(clients[i].username, recipient) == 0)
                {
                    recipient_index = i;
                    break;
                }
            }

            if (recipient_index != -1)
            {
                snprintf(buffer, sizeof(buffer), "\nFrom %s: %s", client->username, message);
                send(clients[recipient_index].client_socket, buffer, strlen(buffer), 0);
                save_message(client->username, recipient, message, 0);
                deliver_messages(recipient);
            }
            else
            {
                // If the recipient is not found, save the message for later delivery
                save_message(client->username, recipient, message, 0);
                send(client->client_socket, "User not found. Message will be delivered when the user logs in.", strlen("User not found. Message will be delivered when the user logs in."), 0);
            }
        }
    }

    client->client_socket = -1;
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s server_ip port_no\n", argv[0]);
        exit(1);
    }

    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        clients[i].client_socket = -1;
    }

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(atoi(argv[2]));

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, MAX_CLIENTS) == -1)
    {
        perror("Listening failed");
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d...\n", atoi(argv[2]));

    pthread_create(&server_CLI, NULL, &cli_handler, NULL);
    FD_ZERO(&set);
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    while (1)
    {
        FD_ZERO(&set);
        FD_SET(server_socket, &set);
        max_fd = server_socket;
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            // valid
            if (clients[i].client_socket > 0)
            {
                FD_SET(clients[i].client_socket, &set);
                if (clients[i].client_socket > max_fd)
                {
                    max_fd = clients[i].client_socket;
                }
            }
        }

        // Checking for change in fd using select
        int check = select(max_fd + 1, &set, NULL, NULL, &timeout);

        if (check < 0)
        {
            perror("Select error\n");
            continue;
        }

        // New connection on server socket
        if (FD_ISSET(server_socket, &set))
        {
            if ((client_socket = accept(server_socket, (struct sockaddr *)&client_socket, (socklen_t *)&client_addr_len)) < 0)
            {
                perror("Accept error\n");
                continue;
            }

            if (client_count == MAX_CLIENTS)
            {
                send(client_socket, "exit", 256, 0);
                close(client_socket);
                continue;
            }

            // Add new socket to array of sockets
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (clients[i].client_socket == -1)
                {
                    clients[i].client_socket = client_socket;
                    clients[i].status = 1;
                    break;
                }
            }
        }
        // check all client sockets for incoming data
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (FD_ISSET(clients[i].client_socket, &set))
            {
                char name[MAX_USERNAME_LEN];
                char buffer[MAX_MESSAGE_LEN];
                int read_size1 = recv(clients[i].client_socket, name, MAX_USERNAME_LEN, 0);
                int read_size2 = recv(clients[i].client_socket, buffer, MAX_MESSAGE_LEN, 0);
                if (read_size1 == 0 || read_size2 == 0)
                {
                    continue;
                }
                if (read_size1 > 0)
                {
                    name[read_size1] = '\0';
                    strcpy(clients[i].username, name);
                }
                else if (read_size2 > 0)
                {
                    buffer[]
                }
            }
        }
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket == -1)
        {
            perror("Acceptance failed");
            continue;
        }

        char username[MAX_USERNAME_LEN];
        char usernameTrimmed[MAX_USERNAME_LEN];
        bzero(username, MAX_USERNAME_LEN);
        // recv(client_socket, username, sizeof(username), 0);
        read(client_socket, username, MAX_USERNAME_LEN);
        printf("Client '%s' connected.\n", username);
        for (int i = 0; i < client_count; i++)
        {

            strncpy(usernameTrimmed, username, MAX_USERNAME_LEN - 1);
            if (strcmp(clients[i].username, usernameTrimmed) == 0)
            {
                if (clients[i].status == -1)
                {
                    close(client_socket);
                    continue;
                }
            }
        }
        if (client_count >= MAX_CLIENTS)
        {
            printf("Server is full. Connection rejected for client '%s'\n", username);
            close(client_socket);
            continue;
        }

        clients[client_count].client_socket = client_socket;
        strncpy(clients[client_count].username, username, MAX_USERNAME_LEN - 1);
        clients[client_count].username[MAX_USERNAME_LEN - 1] = '\0';
        clients[client_count].status = 1;

        pthread_create(&client_threads[client_count], NULL, handle_client, &clients[client_count]);
        client_count++;
    }
    pthread_join(server_CLI, NULL);
    close(server_socket);

    return 0;
}
