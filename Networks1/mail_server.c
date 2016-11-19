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

#define BACKLOG 5
#define GREETING "Winter is coming"
#define FAIL_MSG "Failure"
#define SUCCESS_MSG "Success"
#define MAX_EMAIL_NUMBER 32000
#define DEFAULT_LISTEN_PORT 6423

bool Authenticate(char* usersFile, int socket, char** user);
char** ExtractRecipients(char* recipients_string, int* amount);

typedef struct email{
    char* to;
    EmailContent* content;
    bool active;
} Email;

typedef struct email_content{
    char* from;
    char** recipients;
    char recipients_string[1000];
    char title[100];
    char text[2000];
} EmailContent;

int main(int argc, char* argv[]) {
    if (argc != 2 && argc != 3){
        // TODO: error
    }

    unsigned short portToListen = DEFAULT_LISTEN_PORT;

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
    char bigBuffer[10*1024];
    char nextCommand[1024];
    char commandParam[1024];
    char *user = (char *)malloc(1024);
    Email* emails = (Email *)malloc(sizeof(Email)*MAX_EMAIL_NUMBER);
    int curr_email = 0;
    int i, msg_id;
    int recipients_amount = 0;

    char recipients_string[1024];
    char title[1024];
    char text[1024];

    char** recipients;
    EmailContent* emailContent;

    while(true){
        printf("accepting...\r\n");
        newSock = accept(mainSocket, NULL, &addrlen);
        printf("Accepted new client\r\n");

        strcpy(buffer, GREETING);
        send(newSock, (const char *)&buffer, sizeof(buffer), 0);

        printf("Authenticating...\r\n");
        if (!Authenticate(usersFile, newSock, &user)){
            strcpy(buffer, FAIL_MSG);
            send(newSock, (const char *)&buffer, sizeof(buffer), 0);
            close(newSock);
            continue;
        }
        printf("Authenticated successfully...\r\n");

        // send authentication successful message to client
        strcpy(buffer, SUCCESS_MSG);
        send(newSock, (const char *)&buffer, sizeof(buffer), 0);

        // connected, accept commands

        recv(newSock, (char*)&buffer, sizeof(buffer), 0);
        sscanf(buffer, "%s %s", nextCommand, commandParam);

        while(strcmp(nextCommand,"QUIT") != 0){
            if (strcmp(nextCommand,"SHOW_INBOX")){
                for (i = 1; i < curr_email; i++){
                    if (emails[i].active && strcmp(emails[i].to, user) == 0){
                        sprintf(buffer, "%d;%s;%s", i, emails[i].content->from, emails[i].content->title);
                        send(newSock, (const char *)&buffer, sizeof(buffer), 0);
                    }
                }
                //////////////////////////////ADDED///////////////////////////////////
                sprintf(buffer, "END");
                send(newSock, (const char *)&buffer, sizeof(buffer), 0);
                //////////////////////////////END OF ADDED///////////////////////////////////
            } else if (strcmp(nextCommand,"GET_MAIL")){
                msg_id = atoi(commandParam);
                if (emails[msg_id].active && strcmp(emails[msg_id].to, user) == 0){
                    sprintf(bigBuffer, "%s;%s;%s;%s", emails[msg_id].content->from, emails[msg_id].content->recipients_string,
                            emails[msg_id].content->title, emails[msg_id].content->text);
                    send(newSock, (const char *)&bigBuffer, sizeof(bigBuffer), 0);
                } else {
                    strcpy(buffer, FAIL_MSG);
                    send(newSock, (const char *)&buffer, sizeof(buffer), 0);
                }
            } else if (strcmp(nextCommand,"DELETE_MAIL")){
                if (strcmp(user, emails[atoi(commandParam)].to) == 0){
                    emails[atoi(commandParam)].active = false;
                    strcpy(buffer, SUCCESS_MSG);
                } else {
                    strcpy(buffer, FAIL_MSG);
                }
                send(newSock, (const char *)&buffer, sizeof(buffer), 0);
            } else if (strcmp(nextCommand,"COMPOSE")){
                //////////////////////////////ADDED///////////////////////////////////
                recv(newSock, (char*)&buffer, sizeof(buffer), 0);
                sscanf(buffer, "%s", commandParam);
                //////////////////////////////END OF ADDED///////////////////////////////////
                sscanf(commandParam, "%s;%s;%s", recipients_string, title, text);

                recipients = ExtractRecipients(recipients_string, &recipients_amount);

                emailContent = (EmailContent*)malloc(sizeof(EmailContent));

                strcpy(emailContent->from, user);
                strcpy(emailContent->title, title);
                strcpy(emailContent->text, text);
                strcpy(emailContent->recipients_string, recipients_string);
                emailContent->recipients = recipients;

                // foreach recipient create new email instance
                for (i = 0; i < recipients_amount; i++){
                    curr_email++;
                    strcpy(emails[curr_email].to, recipients[i]);
                    emails[curr_email].content = emailContent;
                    emails[curr_email].active = true;
                }

                strcpy(buffer, SUCCESS_MSG);
                send(newSock, (const char *)&buffer, sizeof(buffer), 0);
            }

            recv(newSock, (char*)&buffer, sizeof(buffer), 0);
            sscanf(buffer, "%s", nextCommand);
        }

        close(newSock);
    }
}

bool Authenticate(char* usersFile, int socket, char** user){
    size_t len = 1024;
    char buffer[1024];
    char checkUsername[1024];
    char checkPassword[1024];
    char username[1024];
    char password[1024];

    // receive authentication data from client
    recv(socket, buffer, len, 0);
    sscanf(buffer, "%s;%s", username, password);
    strcpy(*user, username);

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

char** ExtractRecipients(char* recipients_string, int* amount){
    char** recipients = (char**)malloc(sizeof(char*)*20);
    int i, j, cnt;
    char curr_recipient[1024];
    cnt = 0;

    for (i = 0; i < strlen(recipients_string); i++){
        if (recipients_string[i] == ','){
            curr_recipient[j] = '\0';
            recipients[cnt] = (char*)malloc(j);
            strcpy(recipients[cnt],curr_recipient);
            cnt++;
            j=0;
            continue;
        }
        curr_recipient[j] = recipients_string[i];
        j++;
    }

    curr_recipient[j] = '\0';
    recipients[cnt] = (char*)malloc(j);
    strcpy(recipients[cnt],curr_recipient);
    cnt++;
    *amount = cnt;

    return recipients;
}
