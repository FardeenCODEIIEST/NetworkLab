#include <stdio.h>      // For printf(),scanf(),...
#include <stdlib.h>     // For atoi(), exit() ,...
#include <unistd.h>     // For gethostname(),...
#include <sys/socket.h> // For socket(),..
#include <string.h>     // For strncmp(), bzero(),bcopy() ...
#include <netinet/in.h> // For struct sockaddr_in ,...
#include <errno.h>      // For perror(),...
#include <sys/types.h>  // For definition of structs
#include <netdb.h>      // For hostnet (knowing the IPv4 address)
#include <pthread.h>    // For parallel thread creation

volatile int state = 1; // 1 --> running, 0 --> stop

struct ThreadArgs
{
    int sockfd;
    char buffer[256];
};

void error(const char *err)
{
    perror(err);
    exit(1);
}

void *writeToServer(void *arg)
{
    struct ThreadArgs *args = (struct ThreadArgs *)arg;
    int sockfd = args->sockfd;
    char *buffer = args->buffer;
    while (state)
    {
        bzero(buffer, 256);
        fgets(buffer, 256, stdin);

        int n = write(sockfd, buffer, strlen(buffer));
        if (n < 0)
        {
            state = 0;
            error("Error on writing\n");
        }
        int i = strncmp("Bye", buffer, 3);
        if (i == 0)
        {
            state = 0;
        }
        if (!state)
            break;
    }
    printf("Connection closed\n");
    exit(EXIT_SUCCESS);
    return NULL;
}

void *readFromServer(void *arg)
{
    struct ThreadArgs *args = (struct ThreadArgs *)arg;
    int sockfd = args->sockfd;
    char *buffer = args->buffer;
    while (state)
    {
        bzero(buffer, 256);
        int n = read(sockfd, buffer, 255);
        if (n < 0)
        {
            state = 0;
            error("Error on reading \n");
        }
        printf("Server sent the text: %s\n", buffer);
        int i = strncmp("Bye", buffer, 3);
        if (i == 0)
        {
            state = 0;
        }
        if (!state)
            break;
    }
    printf("Connection closed\n");
    exit(EXIT_SUCCESS);
    return NULL;
}

int main(int argc, char *argv[])
{
    /*
        Usage:- [executable_name] server_ip_address portno
    */
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s server_ip_address port_no\n", argv[0]);
        exit(1);
    }
    /*
        Order:- socket(), connect() , read() <--> write(), close()
    */
    int portNo;                   // Storing the port number
    int sockfd;                   // File descriptor responsible for reading and writing data after succesful connection
    int n;                        // storing the return value of the read(), write() function calls
    char buffer[256];             // For storing data bytes
    struct sockaddr_in serv_addr; // server internet address structures
    struct hostent *server;       // struct for storing data entry of host

    /* socket() */
    portNo = atoi(argv[2]);
    // We will be TCP so SOCK_STREAM
    sockfd = socket(AF_INET, SOCK_STREAM, 0); // 0 --> TCP
    if (sockfd < 0)
    {
        error("Error on opening socket\n");
    }

    /* connect() */
    server = gethostbyname(argv[1]);
    if (server == NULL)
    {
        fprintf(stderr, "Error, no such host\n");
        exit(1);
    }
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr_list[0], (char *)&serv_addr.sin_addr.s_addr, server->h_length); // bcopy(char* src,char* dest,sizeof(src))
    serv_addr.sin_port = htons(portNo);
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        error("Error on connecion\n");
    }
    printf("Connection with server established\nStart the chat session :)\n");
    /* Read and Write */
    while (state)
    {
        struct ThreadArgs args;
        args.sockfd = sockfd;

        pthread_t readThread, writeThread;

        if (pthread_create(&readThread, NULL, readFromServer, (void *)&args) != 0)
        {
            perror("Failed to create read thread\n");
            return 0;
        }

        if (pthread_create(&writeThread, NULL, writeToServer, (void *)&args) != 0)
        {
            perror("Failed to create write thread\n");
            return 0;
        }

        pthread_join(readThread, NULL);
        pthread_join(writeThread, NULL);
    }
    close(sockfd);
    return 0;
}
