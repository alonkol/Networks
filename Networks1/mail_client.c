#include <sys/types.h>
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

#define DEFAULT_PORT "6423"
#define DEFAULT_HOST "localhost"
#define SUCCESS_MSG "Success"


int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 4){
        // TODO: error
    }
    char portToConnect[1024];
    char hostname[1024];

    if (argc==4){
        strcpy(hostname, argv[2]);
        strcpy(portToConnect,argv[3]);
    }else if (argc==3){
        strcpy(portToConnect,DEFAULT_PORT);
        strcpy(hostname, argv[2]);
    }else{
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

    printf("binding...\r\n");
    errcheck = bind(sock,res->ai_addr,res->ai_addrlen);
    if (errcheck == -1){
        printf("Error in function: bind()\r\n"
                       "With error: %s\r\n", strerror(errno));
        return -1;
    }

    // TODO: add connect() here

    printf("Receiving greeting...\r\n");

    char buffer[1024];
    char bigBuffer[10*1024];

    recv(sock, (char*)&buffer, sizeof(buffer), 0);
    printf("greeting: %s\r\n",buffer);

    char user[1024];
    char password[1024];

    printf("Waiting for username and password\r\n");
    scanf("User: %s\n", user);
    scanf("Password: %s\n", password);

    printf("Sending username and password...\r\n");
    sscanf(buffer, "%s;%s", user, password);
    send(sock, (const char *)&buffer, sizeof(buffer), 0);

    printf("Waiting for server to react....\r\n");
    recv(sock, (char*)&buffer, sizeof(buffer), 0);

    if (strcmp(buffer,SUCCESS_MSG)!=0){
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

    while(true){
        printf("Enter Command:\r\n");
        scanf("%s",buffer);
        send(sock, (const char *)&buffer, sizeof(buffer), 0);

        if (strcmp(buffer,"SHOW_INBOX")){
            recv(sock, (char*)&buffer, sizeof(buffer), 0);
            while (strcmp(buffer,"END")!=0){
                printf("%s", buffer);
                recv(sock, (char*)&buffer, sizeof(buffer), 0);
            }
        }
        else if (strcmp(buffer,"GET_MAIL")){
            recv(sock, (char*)&bigBuffer, sizeof(bigBuffer), 0);
            sscanf("%s;%s;%s;%s",from,to,subject,content);
            printf("From: %s\nTo: %s\nSubject: %s\nText: %s\n",from,to,subject,content);
        }
        else if (strcmp(buffer,"DELETE_MAIL")){
            recv(sock, (char*)&buffer, sizeof(buffer), 0);
            printf("%s",buffer);
        }
        else if (strcmp(buffer,"Compose")){
            scanf("To: %s", to);
            scanf("Subject: %s", subject);
            scanf("Text: %s", content);
            sprintf(bigBuffer,"%s;%s;%s", to,subject,content);
            send(sock, (const char *)&bigBuffer, sizeof(bigBuffer), 0);
            recv(sock, (char*)&buffer, sizeof(buffer), 0);
            printf("%s",buffer);
        }
        else if (strcmp(buffer,"Quit")){
            break;
        }
    }

    close(sock);
    freeaddrinfo(res);
}

// TODO if failMSG return
// TODO remove prints we dont need
// TODO: add error handling for every send() & recv()

