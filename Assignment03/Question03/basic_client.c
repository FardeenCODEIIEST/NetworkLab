#include <stdio.h>     // for printf(), ...
#include <stdlib.h>    // for atoi(), ...
#include <string.h>    // for strncmp(),bzero(),bcopy() ...
#include <unistd.h>    // for close(),..
#include <arpa/inet.h> // for inet_addr(),...
#include <pthread.h>   // for pthread_create(),..

#define BUFFER_SIZE 256

int sock;                 // client socket descriptor
pthread_t receive_thread; // reception thread

/*
 *  @param:- const char*: err message
 *  @return:- void
 * function displaying error message
 */
void error(const char *err)
{
    perror(err);
    exit(1);
}

/*
    @param:- void*:---> arguments
    @return:- void*:----> NULL
    functionality:----> handles reception of messages from server
*/
void *receive_handler(void *arg)
{
    char buffer[BUFFER_SIZE];
    int read_size;

    while ((read_size = recv(sock, buffer, BUFFER_SIZE, 0)) > 0)
    {
        buffer[read_size] = '\0';
        printf("\nPartner sent the message: %s\n", buffer);
        if (strncmp(buffer, "Busy", 4) == 0)
        {
            printf("Server is busy, try again later\n");
            break;
        }
        else if (strncmp(buffer, "Bye", 3) == 0)
        {
            printf("Partner has disconnected.\n");
            break;
        }
    }

    close(sock);
    exit(0);
}

int main(int argc, char *argv[])
{

    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s server_ip port_no\n", argv[0]);
        exit(1);
    }
    // socket(), connect(), do stuff , close()
    struct sockaddr_in server;
    char message[BUFFER_SIZE];

    // socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        printf("Could not create socket");
        return 1;
    }

    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_family = AF_INET;
    server.sin_port = htons(atoi(argv[2]));

    // connect
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        error("Connect failed. Error");
        return 1;
    }

    // create reception thread
    pthread_create(&receive_thread, NULL, receive_handler, NULL);
    printf("Chat session has started, you can talk to your partner now.\n");
    while (1)
    {
        bzero(message, BUFFER_SIZE);
        fgets(message, BUFFER_SIZE, stdin);
        message[strcspn(message, "\n")] = 0;
        printf("\n");
        send(sock, message, strlen(message), 0);
        if (strncmp(message, "Bye", 3) == 0)
        {
            break;
        }
    }
    pthread_join(receive_thread, NULL);
    // close
    close(sock);
    return 0;
}
