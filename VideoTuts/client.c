#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> // for hostnet( keeping track of host IPv4 address)

void error(const char *errmsg)
{
    perror(errmsg);
    exit(1);
}

/**
 *  argv[0] --> filename
 *  argv[1] --> server_ipaddress
 *  argv[2] --> port_no
 *
 */

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s hostname port\n", argv[0]);
        exit(1);
    }
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[255];
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        error("Error opening socket\n");
        exit(1);
    }
    server = gethostbyname(argv[1]);
    if (server == NULL)
    {
        fprintf(stderr, "Error, no such host\n");
        exit(1);
    }
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr_list[0], (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        error("Connection failed\n");
    }
    while (1)
    {
        bzero(buffer, 255);
        fgets(buffer, 255, stdin);
        n = write(sockfd, buffer, sizeof(buffer));
        if (n < 0)
        {
            error("Error on writing\n");
            exit(1);
        }
        bzero(buffer, 255);
        n = read(sockfd, buffer, 255);
        if (n < 0)
        {
            error("Error on reading\n");
            exit(1);
        }
        printf("Server: %s\n", buffer);
        int i = strncmp("Bye", buffer, 3);
        if (i == 0)
        {
            break; // bye
        }
    }
    close(sockfd);
    return 0;
}