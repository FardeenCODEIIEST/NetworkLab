#include <stdio.h>     // for printf(), ...
#include <stdlib.h>    // for atoi(), ...
#include <string.h>    // for strncmp(),bzero(),bcopy() ...
#include <unistd.h>    // for close(),..
#include <arpa/inet.h> // for inet_addr(),...
#include <pthread.h>   // for pthread_create(),..

#define MAX_CLIENTS 10
#define BUFFER_SIZE 256

struct client_info
{
    int sock;          // sockfd of the client
    char id[50];       // client id
    int partner_index; // target partner index
};

struct client_info clients[MAX_CLIENTS];
int client_count = 0;

/*
    @param:- int: index of the client in the clients[] array
    @return: void

*/
void close_client_connection(int index)
{
    if (index >= 0 && index < MAX_CLIENTS)
    {
        close(clients[index].sock);
        clients[index].sock = -1; // Marking as closed
        client_count--;           // Decrement client count
    }
}

/*
    @param:- void*
    @return:- void*
    Thread handler for client
*/
void *handle_client(void *arg)
{
    // Retrieve parameters
    int sock = *(int *)arg;
    char buffer[BUFFER_SIZE];
    bzero(buffer, BUFFER_SIZE);
    int read_size;

    // Get the client index
    int client_index = -1;
    for (int i = 0; i < client_count; i++)
    {
        if (clients[i].sock == sock)
        {
            client_index = i;
            break;
        }
    }

    // Read the message from client
    while ((read_size = recv(sock, buffer, BUFFER_SIZE, 0)) > 0)
    {
        buffer[read_size] = '\0';

        if (strncmp(buffer, "Bye", 3) == 0)
        {
            int partner_index = clients[client_index].partner_index;
            if (partner_index != -1)
            {
                // send Bye message to corresponding partner
                send(clients[partner_index].sock, "Bye", 3, 0);
                close_client_connection(partner_index);
            }
            close_client_connection(client_index);
            break;
        }

        // Forward the message to the partner
        int partner_index = clients[client_index].partner_index;
        if (partner_index != -1 && clients[partner_index].sock != -1)
        {
            send(clients[partner_index].sock, buffer, read_size, 0);
        }
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s server_ip port_no\n", argv[0]);
        exit(1);
    }
    // socket(), bind(), listen(), accept(), do stuff , close()
    int server_sock, client_sock, c; // server_sock for listening, client_sock for read,write
    struct sockaddr_in server, client;
    pthread_t thread_id;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1)
    {
        perror("Could not create socket");
        return 1;
    }
    printf("Socket has been established\n");

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_port = htons(atoi(argv[2]));

    if (bind(server_sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("Bind failed");
        return 1;
    }
    printf("Server IP is %s and is listening on port %d\n", argv[1], atoi(argv[2]));

    listen(server_sock, 5);

    c = sizeof(struct sockaddr_in);
    // Accept as many connections as possible till MAX_CLIENTS
    while ((client_sock = accept(server_sock, (struct sockaddr *)&client, (socklen_t *)&c)))
    {
        // handling max_client connections
        if (client_count == MAX_CLIENTS)
        {
            send(client_sock, "Busy", 30, 0); // Server is busy
            close(client_sock);
            continue;
        }
        else if (client_count < MAX_CLIENTS)
        {
            /* Idea is that when clients are connected to server they are automatically paired up with their corresponding partners,
                based on the fact that if the client count is even then partner index is client_count+1, and if odd then
                partner_index is client_count-1.
                An advanced version where the clients get to choose and reject their partners is given in the ./CLI folder
            */
            clients[client_count]
                .sock = client_sock;
            clients[client_count].partner_index = (client_count % 2 == 0) ? client_count + 1 : client_count - 1;
            client_count++;
        }

        // Create a thread for every client.
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

    close(server_sock);

    return 0;
}

/*
    TODO:-
        1. Handling of not sent message
        2. Handling talikng to multiple clients(not at the same time, but the ability to switch clients)

    Intuition:-
        We can maintain an array of struct where the struct is message-specific i.e the struct should have
        rreceiver_name,sender_name,actual_message,delivery_status
*/
