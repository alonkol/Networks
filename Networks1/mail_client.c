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

#define DEFAULT_PORT 6423
#define DEFAULT_HOST "localhost"

int main(int argc, char* argv[]) {
    if (argc != 2 && argc != 3){
        // TODO: error
    }
    unsigned short portToConnect;
    char hostname[1024];

    if (argc==3){
        sscanf(argv[2], "[%s [%hu]]", hostname, &portToConnect);
    }else{
        portToConnect = DEFAULT_PORT;
        strcpy(hostname, DEFAULT_HOST);
    }

    printf("creating socket...\r\n");
    int sock = socket(PF_INET, SOCK_STREAM, 0);

    printf("assigning variables for connecting...\r\n");
    struct sockaddr_in *dest_addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(portToConnect);
    dest_addr.sin_addr = htonl(hostname);

    printf("connecting...\r\n");
    connect(sock, (struct sockaddr*) &dest_addr,sizeof(struct sockaddr));

    printf("Reciving greeting...\r\n");

    char buffer[1024];
    char bigBuffer[10*1024];

    recv(sock, (char*)&buffer, sizeof(buffer), 0);
    printf("%s",buffer);
    free(hostname);

    char user[1024];
    char password[1024];

    printf("Waiting for username and password\r\n");
    scanf("User: %s", user);
    scanf("Password: %s", password);

    printf("Sending username and password...\r\n");
    sscanf(buffer, "%s;%s", user, password);
    send(sock, (const char *)&buffer, sizeof(buffer), 0);

    printf("Waiting for server to react....\r\n");
    recv(sock, (char*)&buffer, sizeof(buffer), 0);

    if (strcmp(buffer,SUCCESS_MSG)!=0){
        printf("Connection failed....\r\n Exiting....\r\n");
        close(sock);
    }
    printf("Connection established....\r\n");

    char from[1024];
    char to[1024];
    char subject[1024];
    char content[1024];

    while(true)){
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
    free(dest_addr);
}

//TODO if argc not good
// TODO if failMSG return
// TODO remove prints we dont need

