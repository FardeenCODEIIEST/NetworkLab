#include <stdio.h>      // For printf(),scanf(),...
#include <stdlib.h>     // For atoi(), exit() ,...
#include <unistd.h>     // For gethostname(),...
#include <sys/socket.h> // For socket(),..
#include <string.h>     // For strncmp(), bzero(),bcopy() ...
#include <netinet/in.h> // For struct sockaddr_in ,...
#include <errno.h>      // For perror(),...
#include <sys/types.h>  // For definition of structs
#include <arpa/inet.h>  // for inet_addr() ...

void error(const char *err)
{
    perror(err);
    exit(1);
}

int main(int argc, char *argv[])
{
    /*
        Usage:- [executable_name] server_ip portNo
    */
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s server_ip port_no\n", argv[0]);
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
    char clientIP[15];                      // Storing the client IP
    /* socket() ----> Get the file descriptor */
    portNo = atoi(argv[2]);
    // We will be using TCP so SOCK_STREAM
    sockfd = socket(AF_INET, SOCK_STREAM, 0); // 0 --> TCP
    if (sockfd < 0)
    {
        error("Error on opening socket\n");
    }
    printf("Socket has been established\n");
    /* bind() ----> What port and IP am I on ? */
    printf("IP is %s\n", argv[1]);
    bzero((char *)&serv_addr, sizeof(serv_addr)); // clears the serv_addr stream with '\0'
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(portNo);
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        error("Error on binding\n");
    }
    /* listen ---> Will anyone listen to me !!!!! */
    listen(sockfd, 5); // maximum number of clients in the queue = 5
    printf("This is the server running on port: %d\n", portNo);
    /* accept() ----> Thanks for calling, now serving you */
    clientlen = sizeof(cli_addr);
    newSockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clientlen);
    if (newSockfd < 0)
    {
        error("Error on accept\n");
    }

    /* Who is trying to connect to me?*/
    printf("The server is connected to client %s:%d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

    /* Read and Write */
    bzero(buffer, 256); // clear the buffer
    n = read(newSockfd, buffer, 256);
    if (n < 0)
    {
        error("Error on reading\n");
    }
    printf("Client sent the text: %s\n", buffer);
    n = write(newSockfd, buffer, sizeof(buffer));
    if (n < 0)
    {
        error("Error on writing\n");
    }
    printf("Closing the connection........\n");

    /* close() --> Bye bye*/
    int err = close(newSockfd);
    if (err == -1)
    {
        error("Read, write socket not closed properly\n");
    }
    err = close(sockfd);
    if (err == -1)
    {
        error("Connection socket not closed properly\n");
    }
    return 0;
}
