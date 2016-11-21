#include <stddef.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
//#include "utils.h"

#define DEFAULT_PORT "6423"
#define DEFAULT_HOST "localhost"
#define SUCCESS_MSG "Success"

int sendall(int s, char *buf, int *len)
{
    int total = 0; /* how many bytes we've sent */
    int bytesleft = *len; /* how many we have left to send */
    int n;
    while(total < *len)
    {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1)
        {
            break;
        }
        total += n;
        bytesleft -= n;
    }
    *len = total; /* return number actually sent here */
    return n == -1 ? -1:0; /*-1 on failure, 0 on success */
}

int recvall(int s, char *buf, int *len)
{
    int total = 0; /* how many bytes we've recieved */
    int bytesleft = *len; /* how many we have left to recieve */
    int n;
    while(total < *len)
    {
        n = recv(s, buf+total, bytesleft, 0);
        if (n == -1)
        {
            break;
        }
        total += n;
        bytesleft -= n;
    }
    *len = total; /* return number actually sent here */
    return n == -1 ? -1:0; /*-1 on failure, 0 on success */
}


int main(int argc, char* argv[])
{
    if (argc < 2 || argc > 4)
    {
        // TODO: error
    }
    char portToConnect[1024];
    char hostname[1024];

    if (argc==4)
    {
        strcpy(hostname, argv[2]);
        strcpy(portToConnect,argv[3]);
    }
    else if (argc==3)
    {
        strcpy(portToConnect,DEFAULT_PORT);
        strcpy(hostname, argv[2]);
    }
    else
    {
        strcpy(portToConnect,DEFAULT_PORT);
        strcpy(hostname, DEFAULT_HOST);
    }

    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(hostname, portToConnect, &hints, &res );

    printf("creating socket...\r\n");
    int sock = socket(res->ai_family, res->ai_socktype, res ->ai_protocol);
    int errcheck;

    // pretty sure client doesn't need binding...
    /*
    printf("binding...\r\n");
    errcheck = bind(sock,res->ai_addr,res->ai_addrlen);
    if (errcheck == -1){
        printf("Error in function: bind()\r\n"
                       "With error: %s\r\n", strerror(errno));
        return -1;
    }
    */

    printf("connecting...\r\n");
    connect(sock, (struct sockaddr*) &res,sizeof(struct sockaddr));


    printf("Receiving greeting...\r\n");

    char buffer[1024];
    int buffer_size = sizeof(buffer);
    char bigBuffer[10*1024];

    recvall(sock, (char*)&buffer, &buffer_size);
    printf("greeting: %s\r\n",buffer);

    char user[1024];
    char password[1024];

    printf("Waiting for username and password\r\n");
    scanf("User: %s\n", user);
    scanf("Password: %s\n", password);
    print("%s %s\n", user,password);

    printf("sending username and password...\r\n");
    sscanf(buffer, "%s;%s", user, password);
    sendall(sock, (char *)&buffer, &buffer_size);

    printf("Waiting for server to react....\r\n");
    recvall(sock, (char*)&buffer, &buffer_size);

    if (strcmp(buffer,SUCCESS_MSG)!=0)
    {
        printf("Connection failed....\r\n Exiting....\r\n");
        close(sock);
        freeaddrinfo(res);
        return 1;
    }
    printf("Connection established....\r\n");

    char from[1024];
    char to[1024];
    char subject[1024];
    char content[1024];

    while(true)
    {
        printf("Enter Command:\r\n");
        scanf("%s",buffer);
        sendall(sock, (char *)&buffer, &buffer_size);

        if (strcmp(buffer,"SHOW_INBOX"))
        {
            recvall(sock, (char*)&buffer, &buffer_size);
            while (strcmp(buffer,"END")!=0)
            {
                printf("%s", buffer);
                recvall(sock, (char*)&buffer, &buffer_size);
            }
        }
        else if (strcmp(buffer,"GET_MAIL"))
        {
            int bigbuffersize= sizeof(bigBuffer);
            recvall(sock, (char*)&bigBuffer, &bigbuffersize);
            sscanf("%s;%s;%s;%s",from,to,subject,content);
            printf("From: %s\nTo: %s\nSubject: %s\nText: %s\n",from,to,subject,content);
        }
        else if (strcmp(buffer,"DELETE_MAIL"))
        {
            recvall(sock, (char*)&buffer, &buffer_size);
            printf("%s",buffer);
        }
        else if (strcmp(buffer,"Compose"))
        {
            scanf("To: %s", to);
            scanf("Subject: %s", subject);
            scanf("Text: %s", content);
            sprintf(bigBuffer,"%s;%s;%s", to,subject,content);
            sendall(sock, (char *)&buffer, &buffer_size);
            recvall(sock, (char*)&buffer, &buffer_size);
            printf("%s",buffer);
        }
        else if (strcmp(buffer,"Quit"))
        {
            break;
        }
    }

    close(sock);
    freeaddrinfo(res);
}

// TODO if failMSG return
// TODO remove prints we dont need
// TODO: add error handling for every sendall() & recvall()

