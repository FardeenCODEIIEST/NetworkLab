#include <stdio.h>      // for printf(),...
#include <stdlib.h>     // for atoi(),...
#include <string.h>     // for strncmp(),bzero,bcopy(),...
#include <unistd.h>     // forclose()
#include <arpa/inet.h>  // for inet_addr()
#include <sys/socket.h> // for socket(),bind(),listen(),...
#include <sys/select.h> // for select()

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

void close_client_connection(int index)
{
    if (index >= 0 && index < MAX_CLIENTS)
    {
        close(clients[index].sock);
        clients[index].sock = -1; // Marking as closed
        client_count--;           // Decrement client count
    }
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s server_ip port_no\n", argv[0]);
        exit(1);
    }

    int server_sock, client_sock, c;
    struct sockaddr_in server, client;
    fd_set set;
    int max_fd;
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    // Initialize all client sockets to -1
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        clients[i].sock = -1;
    }
    // socket(), bind(), listent(), select()<--->accept(),read(),write(), close()
    // Create socket
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

    // Bind
    if (bind(server_sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("Bind failed");
        return 1;
    }
    printf("Server IP is %s and is listening on port %d\n", argv[1], atoi(argv[2]));

    // Listen
    listen(server_sock, 5);

    c = sizeof(struct sockaddr_in);

    while (1)
    {
        // file descriptor set initialization
        FD_ZERO(&set);
        FD_SET(server_sock, &set);
        max_fd = server_sock;

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (clients[i].sock > 0)
            {
                FD_SET(clients[i].sock, &set);
                if (clients[i].sock > max_fd)
                {
                    max_fd = clients[i].sock;
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
        if (FD_ISSET(server_sock, &set))
        {
            if ((client_sock = accept(server_sock, (struct sockaddr *)&client, (socklen_t *)&c)) < 0)
            {
                perror("Accept error\n");
                continue;
            }

            // handling max_client connections
            if (client_count == MAX_CLIENTS)
            {
                send(client_sock, "Busy", 30, 0); // Server is busy
                close(client_sock);
                continue;
            }

            // Add new socket to array of sockets
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (clients[i].sock == -1)
                {
                    clients[i].sock = client_sock;
                    clients[i].partner_index = (client_count % 2 == 0) ? client_count + 1 : client_count - 1;
                    client_count++;
                    break;
                }
            }
        }

        // Check all clients for incoming data on client socket
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (FD_ISSET(clients[i].sock, &set))
            {
                char buffer[BUFFER_SIZE];
                int read_size = recv(clients[i].sock, buffer, BUFFER_SIZE, 0);

                if (read_size == 0)
                {
                    // Client disconnected
                    close_client_connection(i);
                }
                else if (read_size > 0)
                {
                    // Echo the message back to client
                    buffer[read_size] = '\0';

                    // Process message (e.g., check for "Bye")
                    if (strncmp(buffer, "Bye", 3) == 0)
                    {
                        int partner_index = clients[i].partner_index;
                        if (partner_index != -1)
                        {
                            send(clients[partner_index].sock, "Bye", 3, 0);
                            close_client_connection(partner_index);
                        }
                        close_client_connection(i);
                        continue;
                    }

                    // Forward the message to the partner
                    int partner_index = clients[i].partner_index;
                    if (partner_index != -1 && clients[partner_index].sock != -1)
                    {
                        send(clients[partner_index].sock, buffer, read_size, 0);
                    }
                }
            }
        }
    }
    // close
    close(server_sock);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].sock != -1)
            close(clients[i].sock);
    }
    return 0;
}
