#include <stdbool.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include "utils.h"

int sendall(int s, char *buf)
{
    int total = 0; /* how many bytes we've sent */
    int n;
    // first two bytes will be the message length
    int totalLen = strlen(buf);
    short* len_short = (short*)&totalLen;
    char len_string[2];
    sprintf(len_string,"%2i",*len_short);
    n = send(s, len_string, 2, 0);
    if (n == -1)
    {
        printf("Error in function sendall()\r\n"
               "%s", strerror(errno));
        return -1;
    }

    int bytesleft = totalLen;
    while(total < totalLen)
    {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1)
        {
            printf("Error in function sendall()\r\n"
                   "%s", strerror(errno));
            break;
        }
        total += n;
        bytesleft -= n;
    }
    return n == -1 ? -1:0; /*-1 on failure, 0 on success */
}

int recvall(int s, char *buf)
{
    int total = 0; /* how many bytes we've received */
    int n;
    char len[2];

    // read first two bytes to know how many bytes to read
    n = recv(s,len, 2,0);
    if (n == -1)
    {
        printf("Error in function recvall()\r\n"
               "%s", strerror(errno));
        return -1;
    }
    int totalLen = atoi(len);
    int bytesleft = totalLen;

    while(total < totalLen)
    {
        n = recv(s, buf+total, bytesleft, 0);
        if (n == -1)
        {
            printf("Error in function recvall()\r\n"
                   "%s", strerror(errno));
            break;
        }
        if (n == 0) // client disconnected
        {
            printf("Other side disconnected \r\n");
            return -1;
        }
        total += n;
        bytesleft -= n;
    }
    buf[total]='\0';
    return n == -1 ? -1:0; /*-1 on failure, 0 on success */
}

void safe_shutdown(int newSock, char* buffer)
{
    shutdown(newSock, SHUT_WR);
    int res = 1;
    while(res > 0) { // if no more data to read, or error in reading - close socket
    res = recv(sock, buffer, 4000);
    }
    close(newSock);
}

