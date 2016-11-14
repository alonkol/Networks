#include <sys/types.h>
#include <stddef.h>
#include <stdbool.h>
#include <sys/socket.h>

#include <stdlib.h>
#include <stdio.h>

// TODO: remove later
/*
#include <winsock.h>
#include <ws2tcpip.h>
#include <io.h>
*/

#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#define BACKLOG 5
#define GREETING "Winter is coming"
#define AUTH_FAIL_MSG "Failure"
#define AUTH_SUCCESS_MSG "Success"

bool authenticate(char* usersFile, int socket);

int main(int argc, char* argv[]) {
    if (argc != 2 && argc != 3){
        // TODO: error
    }

    u_short portToListen = 6423;

    char* usersFile = argv[1];
    if (argc == 3){
        portToListen = (u_short) strtoul(argv[2], NULL, 0);
    }

    printf("creating socket...\r\n");
    int mainSocket = socket(AF_INET, SOCK_STREAM, 0);

    printf("assigning variables for binding...\r\n");

    struct sockaddr_in *my_addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));

    socklen_t addrlen = sizeof(my_addr);

    my_addr->sin_family = AF_INET;
    my_addr->sin_addr.s_addr = htonl(INADDR_ANY);
    my_addr->sin_port = htons(portToListen);

    printf("binding...\r\n");
    bind(mainSocket, (struct sockaddr*) &my_addr, addrlen);
            // TODO: error

    printf("listening...\r\n");
    listen(mainSocket, BACKLOG);

    int newSock;
    char buffer[1024];
    char nextCommand[1024];

    while(true){
        printf("accepting...\r\n");
        newSock = accept(mainSocket, NULL, &addrlen);
        printf("Accepted new client\r\n");

        strcpy(buffer, GREETING);
        send(newSock, (const char *)&buffer, sizeof(buffer), 0);

        printf("Authenticating...\r\n");
        if (!authenticate(usersFile, newSock)){
            strcpy(buffer, AUTH_FAIL_MSG);
            send(newSock, (const char *)&buffer, sizeof(buffer), 0);
            close(newSock);
            continue;
        }
        printf("Authenticated successfully...\r\n");

        // send authentication successful message to client
        strcpy(buffer, AUTH_SUCCESS_MSG);
        send(newSock, (const char *)&buffer, sizeof(buffer), 0);

        // connected, accept commands

        recv(newSock, (char*)&buffer, sizeof(buffer), 0);
        sscanf(buffer, "%s", nextCommand);

        while(strcmp(nextCommand,"QUIT") != 0){
            // TODO: Handle commands

            recv(newSock, (char*)&buffer, sizeof(buffer), 0);
            sscanf(buffer, "%s", nextCommand);
        }

        close(newSock);
    }
}

bool authenticate(char* usersFile, int socket){
    size_t len = 1024;
    char buffer[1024];
    char checkUsername[1024];
    char checkPassword[1024];
    char username[1024];
    char password[1024];

    // receive authentication data from client
    recv(socket, buffer, len, 0);
    sscanf(buffer, "%s;%s", username, password);

    // read form file
    FILE* fp = fopen(usersFile, "r");

    while(fgets(buffer, 1024, fp) != NULL){
        sscanf(buffer, "%s\t%s", checkUsername, checkPassword);
        if (strcmp(checkUsername, username) == 0){
            return true;
        }
    }
    return false;

}