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
#include <sys/select.h> // FOr select() ,...

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

/*
    function used to read text from server
    @params:- void*
    @return:- void*
*/
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
        if (state)
        {
            printf("Server sent the text: %s\n", buffer);
        }
        int i = strncmp("Bye", buffer, 3);
        if (i == 0)
        {
            state = 0;
        }
        if (!state)
            break;
    }
    // printf("Connection closed\n");
    // exit(EXIT_SUCCESS);
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
    printf("Connecting to %s:%s\n", argv[1], argv[2]);
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        error("Error on connecion\n");
    }
    printf("Connection with server established\nStart the chat session :)\n");
    /* Read and Write */
    struct ThreadArgs args;
    args.sockfd = sockfd;

    pthread_t readThread; // thread for reading text from server

    if (pthread_create(&readThread, NULL, readFromServer, (void *)&args) != 0)
    {
        perror("Failed to create read thread\n");
        return 0;
    }

    // while (state)
    // {
    //     bzero(buffer, 256);
    //     fgets(buffer, 256, stdin);

    //     int n = write(sockfd, buffer, strlen(buffer));
    //     printf("\n");
    //     if (n < 0)
    //     {
    //         state = 0;
    //         error("Error on writing\n");
    //     }
    //     int i = strncmp("Bye", buffer, 3);
    //     if (i == 0)
    //     {
    //         state = 0;
    //     }
    //     if (!state)
    //         break;
    // }--------------------------------------------------> This is blocking if we do not write exit(EXIT_SUCCESS) in the reception thread
    while (state)
    {
        fd_set set;
        struct timeval timeout;

        // Initialize the file descriptor set
        FD_ZERO(&set);
        FD_SET(STDIN_FILENO, &set);

        // Setting timeout to zero, which makes select() non-blocking
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;

        // Check if we can read from stdin without blocking
        int rv = select(STDIN_FILENO + 1, &set, NULL, NULL, &timeout);
        if (rv == -1)
        {
            perror("select"); // Error occurred in select()
        }
        else if (rv == 0)
        {
            // No data to read
        }
        else
        {
            // Data is available to read
            bzero(buffer, 256);
            if (fgets(buffer, 256, stdin) != NULL)
            {
                int n = write(sockfd, buffer, strlen(buffer));
                printf("\n");
                if (n < 0)
                {
                    state = 0;
                    error("Error on writing\n");
                }
                int i = strncmp("Bye", buffer, 3);
                if (i == 0)
                {
                    state = 0;
                    break;
                }
            }
        }
    }

    // pthread_join(readThread, NULL);

    /* close()*/
    if (state == 0)
    {
        int err = close(sockfd);
        if (err == -1)
        {
            error("Close is not successful\n");
        }
        printf("Connection with server closed\n");
    }
    return 0;
}
