#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

void error(const char *errmsg)
{
    perror(errmsg);
    exit(1);
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Port number not provided, program terminated\n");
        exit(1);
    }
    int sockfd, newsockfd, portno, n;
    char buffer[255];
    portno = atoi(argv[1]);
    struct sockaddr_in serv_addr, cli_addr; // internet address
    socklen_t clilen;                       // 32-bit datatype

    sockfd = socket(AF_INET, SOCK_STREAM, 0); // TCP
    if (sockfd < 0)
    {
        error("Error opening socket\n");
        exit(1);
    }
    bzero((char *)&serv_addr, sizeof(serv_addr)); // erases data by writing '\0' to the stream of bytes referred by pointer
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        error("Binding failer\n");
    }

    listen(sockfd, 5); // Max number of clients=5
    clilen = sizeof(cli_addr);

    // Read, write
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    if (newsockfd < 0)
    {
        error("Error on accept\n");
        exit(1);
    }
    while (1)
    {
        bzero(buffer, 255); // clear the buffer
        n = read(newsockfd, buffer, 255);
        if (n < 0)
        {
            error("Error on reading\n");
            exit(1);
        }
        printf("Client request: %s\n", buffer);
        bzero(buffer, 255);        // clear the buffer
        fgets(buffer, 255, stdin); // reads bytes from stream and feed into buffer

        n = write(newsockfd, buffer, strlen(buffer));
        if (n < 0)
        {
            error("Error on writing\n");
            exit(1);
        }
        int i = strncmp("Bye", buffer, 3);
        if (i == 0)
        {
            break; // bye
        }
    }
    close(newsockfd);
    close(sockfd);
    return 0;
}
