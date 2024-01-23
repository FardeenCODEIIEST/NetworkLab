/**
 *  From stackoverflow, used the concept of mutex_lock and mutex_unlock can alos try mutex_trylock
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define PORT 9666

struct client_info
{
    int sock;
    char id[50];
};

struct client_info clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;

void *handle_client(void *arg)
{
    int sock = *(int *)arg;
    char buffer[BUFFER_SIZE];
    char target_id[50];
    int read_size;
    char *message;

    if ((read_size = recv(sock, target_id, 50, 0)) > 0)
    {
        target_id[read_size] = '\0';
        pthread_mutex_lock(&client_mutex);
        strcpy(clients[client_count].id, target_id);
        clients[client_count++].sock = sock;
        pthread_mutex_unlock(&client_mutex);
    }
    else
    {
        close(sock);
        return NULL;
    }

    while ((read_size = recv(sock, buffer, BUFFER_SIZE, 0)) > 0)
    {
        buffer[read_size] = '\0';
        message = strchr(buffer, ' ');
        if (message != NULL)
        {
            *message = '\0';
            message++;
            strcpy(target_id, buffer);

            pthread_mutex_lock(&client_mutex);
            for (int i = 0; i < client_count; i++)
            {
                if (strcmp(clients[i].id, target_id) == 0)
                {
                    send(clients[i].sock, message, strlen(message), 0);
                    break;
                }
            }
            pthread_mutex_unlock(&client_mutex);
        }
    }

    close(sock);
    return NULL;
}

int main()
{
    int server_sock, client_sock, c;
    struct sockaddr_in server, client;
    pthread_t thread_id;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1)
    {
        perror("Could not create socket");
        return 1;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("Bind failed");
        return 1;
    }

    listen(server_sock, 3);

    c = sizeof(struct sockaddr_in);
    while ((client_sock = accept(server_sock, (struct sockaddr *)&client, (socklen_t *)&c)))
    {
        if (pthread_create(&thread_id, NULL, handle_client, (void *)&client_sock) < 0)
        {
            perror("Could not create thread");
            return 1;
        }
    }

    if (client_sock < 0)
    {
        perror("Accept failed");
        return 1;
    }

    return 0;
}
