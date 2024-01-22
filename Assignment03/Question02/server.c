#include <stdio.h>      // For printf(),scanf(),...
#include <stdlib.h>     // For atoi(), exit() ,...
#include <unistd.h>     // For gethostname(),...
#include <sys/socket.h> // For socket(),..
#include <string.h>     // For strncmp(), bzero(),bcopy() ...
#include <netinet/in.h> // For struct sockaddr_in ,...
#include <errno.h>      // For perror(),...
#include <sys/types.h>  // For definition of structs
#include <pthread.h>    // For parallel thread creation

pthread_t tid[2];

void error(const char *err)
{
    perror(err);
    exit(1);
}

void *readingTextFromCLient(char buffer[256], int newSockfd)
{
    bzero(buffer, 256); // clear the buffer
    int n = read(newSockfd, buffer, 256);
    if (n < 0)
    {
        error("Error on reading\n");
    }
    printf("Client sent the text: %s\n", buffer);
    return NULL;
}

void *writingTextToClient(char buffer[256], int newSockfd, int sockfd)
{
    bzero(buffer, 256);
    fgets(buffer, 256, stdin); // taking input from stdin stream and feed into buffer
    int n = write(newSockfd, buffer, strlen(buffer));
    if (n < 0)
    {
        error("Error on writing\n");
    }
    int i = strncmp("Bye", buffer, 3); // comparing 3 characters
    if (i == 0)
    {
        printf("Closing the connection ....\n");
        close(newSockfd);
        close(sockfd);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    /*
        Usage:- [executable_name] portNo
    */
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s port_no\n", argv[0]);
        exit(1);
    }
    /*
        Order:- socket(), bind(), listen(), accept(), read() <--> write(), close()
    */
    int portNo;                             // Storing the port number
    int sockfd;                             // File descriptor responsible for listening to new connections
    int newSockfd;                          // File descriptor responsible for read() and write()
    int n;                                  // storing the return value of the read(), write() calls
    char buffer[256];                       // For storing data bytes
    struct sockaddr_in serv_addr, cli_addr; // server and client internet address structures
    socklen_t clientlen;                    // 4 byte datatype

    /* socket() */
    portNo = atoi(argv[1]);
    // We will be TCP so SOCK_STREAM
    sockfd = socket(AF_INET, SOCK_STREAM, 0); // 0 --> TCP
    if (sockfd < 0)
    {
        error("Error on opening socket\n");
    }

    /* bind() */
    bzero((char *)&serv_addr, sizeof(serv_addr)); // clears the serv_addr stream with '\0'
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portNo);
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        error("Error on binding\n");
    }

    /* listen*/
    listen(sockfd, 5); // maximum number of clients=5

    /* accept() */
    clientlen = sizeof(cli_addr);
    newSockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clientlen);
    if (newSockfd < 0)
    {
        error("Error on accept\n");
    }

    /* Read and Write*/
    printf("This is the server running on port: %d\n", portNo);
    n = pthread_create(&(tid[0]), NULL, readingTextFromCLient(buffer, newSockfd), NULL);
    if (n != 0)
    {
        fprintf(stderr, "Read thread cannot be created sccessfully\n");
        exit(1);
    }
    n = pthread_create(&(tid[1]), NULL, writingTextToClient(buffer, newSockfd, sockfd), NULL);
    if (n != 0)
    {
        fprintf(stderr, "Write thread cannot be created sccessfully\n");
        exit(1);
    }
    // printf("Closing the connection ....\n");

    /* close() */
    // end:
    //     close(newSockfd);
    //     close(sockfd);
    //     return 0;
    return 0;
}