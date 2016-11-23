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
//#include "utils.h"

#define BACKLOG 5
#define GREETING "Winter is coming"
#define FAIL_MSG "Failure"
#define SUCCESS_MSG "Success"
#define MAX_EMAIL_NUMBER 32000
#define DEFAULT_LISTEN_PORT 6899

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


bool Authenticate(char* usersFile, int socket, char** user);
char** ExtractRecipients(char* recipients_string, int* amount);

typedef struct email_content{
    char* from;
    char** recipients;
    char recipients_string[1000];
    char title[100];
    char text[2000];
} EmailContent;

typedef struct email{
    char* to;
    EmailContent* content;
    bool active;
} Email;

int main(int argc, char* argv[]) {
    if (argc != 2 && argc != 3){
        printf("Invalid input. please use the following format: \r\n"
                       "mail_server <users file> <optional:port>\r\n");
        return -1;
    }

    int errcheck;

    unsigned short portToListen = DEFAULT_LISTEN_PORT;


    char* usersFile = argv[1];
    if (argc == 3){
        portToListen = (u_short) strtoul(argv[2], NULL, 0);
    }

    printf("creating socket...\r\n");
    int mainSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (mainSocket == -1){
        printf("Error in function: socket()\r\n"
                       "With error: %s\r\n", strerror(errno));
        return -1;
    }

    printf("assigning variables for binding...\r\n");

    struct sockaddr_in my_addr;

    socklen_t addrlen = sizeof(my_addr);

    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    my_addr.sin_port = htons(portToListen);



    printf("binding...\r\n");
    errcheck = bind(mainSocket, (const struct sockaddr*)&my_addr, addrlen);
    if (errcheck == -1){
        printf("Error in function: bind()\r\n"
                       "With error: %s\r\n", strerror(errno));
        return -1;
    }

    printf("listening...\r\n");
    errcheck = listen(mainSocket, BACKLOG);
    if (errcheck == -1){
        printf("Error in function: listen()\r\n"
                       "With error: %s\r\n", strerror(errno));
        return -1;
    }


    int newSock;
    char buffer[1024];
    char bigBuffer[10*1024];
    char nextCommand[1024];
    char commandParam[1024];
    int buffersize = sizeof(buffer);
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
        sendall(newSock, (char *)&buffer, &buffersize);

        printf("Authenticating...\r\n");
        if (!Authenticate(usersFile, newSock, &user)){
            strcpy(buffer, FAIL_MSG);
            sendall(newSock, (char *)&buffer, &buffersize);
            close(newSock);
            continue;
        }
        printf("Authenticated successfully...\r\n");

        // sendall authentication successful message to client
        strcpy(buffer, SUCCESS_MSG);
        sendall(newSock, (char *)&buffer, &buffersize);

        // connected, accept commands

        recvall(newSock, (char*)&buffer, &buffersize);
        sscanf(buffer, "%s %s", nextCommand, commandParam);

        while(strcmp(nextCommand,"QUIT") != 0){
            if (strcmp(nextCommand,"SHOW_INBOX")){
                for (i = 1; i < curr_email; i++){
                    if (emails[i].active && strcmp(emails[i].to, user) == 0){
                        sprintf(buffer, "%d;%s;%s", i, emails[i].content->from, emails[i].content->title);
                        sendall(newSock, (char *)&buffer, &buffersize);
                    }
                }
                sprintf(buffer, "END");
                sendall(newSock, (char *)&buffer, &buffersize);
            } else if (strcmp(nextCommand,"GET_MAIL")){
                msg_id = atoi(commandParam);
                if (emails[msg_id].active && strcmp(emails[msg_id].to, user) == 0){
                    sprintf(bigBuffer, "%s;%s;%s;%s", emails[msg_id].content->from, emails[msg_id].content->recipients_string,
                            emails[msg_id].content->title, emails[msg_id].content->text);
                    int bigbuffersize = sizeof(bigBuffer);
                    sendall(newSock, (char *)&bigBuffer, &bigbuffersize);
                } else {
                    strcpy(buffer, FAIL_MSG);
                    sendall(newSock, (char *)&buffer, &buffersize);
                }
            } else if (strcmp(nextCommand,"DELETE_MAIL")){
                if (strcmp(user, emails[atoi(commandParam)].to) == 0){
                    emails[atoi(commandParam)].active = false;
                    strcpy(buffer, SUCCESS_MSG);
                } else {
                    strcpy(buffer, FAIL_MSG);
                }
                sendall(newSock, (char *)&buffer, &buffersize);
            } else if (strcmp(nextCommand,"COMPOSE")){
                recvall(newSock, (char*)&buffer, &buffersize);
                sscanf(buffer, "%s", commandParam);
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
                sendall(newSock, (char *)&buffer, &buffersize);
            }

            recvall(newSock, (char*)&buffer, &buffersize);
            sscanf(buffer, "%s", nextCommand);
        }

        close(newSock);
    }
}

bool Authenticate(char* usersFile, int socket, char** user){
    int len = 1024;
    char buffer[1024];
    char checkUsername[1024];
    char checkPassword[1024];
    char username[1024];
    char password[1024];

    // receive authentication data from client
    recvall(socket, buffer, &len);
    sscanf(buffer, "%s;%s", username, password);
	printf("got from client: %s\n", buffer);
    strcpy(*user, username);
	printf("username - %s\n",user);
    // read form file
    FILE* fp = fopen(usersFile, "r");

    while(fgets(buffer, 1024, fp) != NULL){
        sscanf(buffer, "%s\t%s", checkUsername, checkPassword);
	printf("buffer: %s\n",buffer);
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

// TODO: add error handling for every sendall() & recvall()
