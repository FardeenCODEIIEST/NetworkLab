#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#define MAX_FILE_NAME_SIZE 100
#define MAX_MODE_SIZE 10
#define MAX_BLOCK_SIZE 512
#define MAX_ERR_MSG_SIZE 100

typedef struct
{
    uint16_t opcode; // tftp opcode:- 1---> RRQ, 2---> WRQ, 3---> DATA, 4---> ACK, 5---> ERROR
    // L5 specifications  ---> RRQ / WRQ
    char fileName[MAX_FILE_NAME_SIZE]; // filename
    char mode[MAX_MODE_SIZE];          // mode in which bytes are present in the file

    // L5 specifications  ---> DATA
    uint16_t blockNumber;      // block number of the data packet
    char data[MAX_BLOCK_SIZE]; // Data payload

    // L5 specifications  ---> ErrMsg
    uint16_t error_code;                //  Error Code
    char err_message[MAX_ERR_MSG_SIZE]; // Error Message

} tftp_packet;

void serialise(char *buffer, tftp_packet *packet)
{
}

void deserialise(char *buffer, tftp_packet *packet)
{
}

int main(int argc, char *argv[])
{
    // Usage:- [exe_name] [server_ip] [server_port] [Method] [File_Name]
    if (argc != 5)
    {
        fprintf(stderr, "Usage:- %s  <server_ip> <server_port> <Method> <File_Name>\n", argv[0]);
        return -2;
    }
    struct sockaddr_in server_addr;
    socklen_t addr_size;
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1)
    {
        perror("socket() failed:\n");
        return 0;
    }
    // server address initialisation
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));

    char *file_name = argv[4];
    char *mode = argv[3];

    // Mode can be :- 1. GET (fetch files from server storage) 2. PUT (write files onto server storage)
    for (int i = 0; i < strlen(mode); i++)
    {
        mode[i] = toupper(mode[i]);
    }
    if (strncmp(mode, "PUT", 3) != 0 && strncmp(mode, "GET", 3) != 0)
    {
        fprintf(stderr, "Incorrect method\n Terminating .....\n");
        return -1;
    }
    printf("You requested for %s method on the file named %s\n", mode, file_name);
    if (strncmp(mode, "PUT", 3) == 0)
    {
        // Write files to the server
        // Open the file
        FILE *fp = fopen(file_name, "r+");
        if (fp == NULL)
        {
            perror("fopen error\n");
            return -1;
        }
        // Send a WRQ request to the server
        tftp_packet *packet_wrq = (tftp_packet *)malloc(sizeof(tftp_packet));
        packet_wrq->opcode = 2;
        strcpy(packet_wrq->fileName, file_name);
        packet_wrq->fileName[strlen(file_name)] = '\0';
        strcpy(packet_wrq->mode, "NETASCII");
        int packet_wrq_len = sizeof(packet_wrq->opcode) + sizeof(packet_wrq->fileName) + sizeof(packet_wrq->mode) + 2;
        char *buffer = (char *)malloc(sizeof(char) * packet_wrq_len);
        serialise(buffer, packet_wrq);
        }
    else
    {
        // Fetch files from server
    }
    return 0;
}