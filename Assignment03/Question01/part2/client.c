#include <stdio.h>      // For printf(),scanf(),...
#include <stdlib.h>     // For atoi(), exit() ,...
#include <unistd.h>     // For gethostname(),...
#include <sys/socket.h> // For socket(),..
#include <string.h>     // For strncmp(), bzero(),bcopy() ...
#include <netinet/in.h> // For struct sockaddr_in ,...
#include <errno.h>      // For perror(),...
#include <sys/types.h>  // For definition of structs
#include <netdb.h>      // for hostnet (knowing the IPv4 address)

void error(const char *err)
{
    perror(err);
    exit(1);
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
    while (1)
    {
        bzero(buffer, 256);
        fgets(buffer, 256, stdin);
        n = write(sockfd, buffer, sizeof(buffer));
        printf("\n");
        int k = strncmp("Bye", buffer, 3);
        if (k == 0)
        {
            break;
        }
        if (n < 0)
        {
            error("Error on writing:\n");
        }
        bzero(buffer, 256);
        n = read(sockfd, buffer, 256);
        if (n < 0)
        {
            error("Error on reading\n");
        }
        printf("Server sent the text: %s\n", buffer);
        int i = strncmp("Bye", buffer, 3);
        if (i == 0)
        {
            break;
        }
    }
    /* close()*/
    int err = close(sockfd);
    if (err == -1)
    {
        error("Close is not successful\n");
    }
    printf("Connection with server closed\n");
    return 0;
}
