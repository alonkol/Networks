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
#include <unistd.h>
#include "utils.h"
#include "mail_server.h"

int main(int argc, char* argv[])
{
    if (argc !=2 && argc != 3)
    {
        printf("Invalid input. please use the following format: \r\n"
               "mail_server <users file> <optional:port>\r\n");
        return -1;
    }

    unsigned short portToListen = DEFAULT_PORT;

    char* usersFile = argv[1];
    if (argc == 3)
    {
        portToListen = (unsigned short) strtoul(argv[2], NULL, 0);
    }

    struct sockaddr_in my_addr;
    socklen_t addrlen = sizeof(my_addr);
    int mainSocket = init_listen(portToListen);
    if (mainSocket == -1)
    {
        return -1;
    }

    int newSock;
    char buffer[SMALL_BUFFER_SIZE], bigBuffer[BIG_BUFFER_SIZE];
    char nextCommand[SMALL_BUFFER_SIZE], commandParam[SMALL_BUFFER_SIZE];
    char* user = (char*)malloc(MAX_USERNAME);
    Email emails[MAXMAILS+1];
    int curr_email = 1, recipients_amount = 0;
    int i, k, msg_id;
    bool breakOuter = false;

    char recipients_string[(MAX_USERNAME+1)*TO_TOTAL];
    char title[MAX_SUBJECT];
    char text[MAX_CONTENT];

    char** recipients;
    EmailContent* emailContent;

    while(true)
    {
        printf("accepting...\r\n");
        newSock = accept(mainSocket, NULL, &addrlen);
        printf("Accepted new client\r\n");

        strcpy(buffer, GREETING);
        if (sendall(newSock, (char *)&buffer) == -1)
        {
            close(newSock);
            continue;
        }

        //  TODO: support multi users
        // connected, load user emails to memory
        int j = 1;
        int active_user_emails[MAXMAILS+1] = {0};
        for (i = 1; i <= curr_email; i++)
        {
            if (strcmp(emails[i].to, user) == 0)
            {
                active_user_emails[j] = i;
                j++;
            }
        }

        // accept commands
        printf("Waiting for command...\r\n");
        if (recvall(newSock, (char*)&buffer) == -1)
        {
            break;
        }
        sscanf(buffer, "%s %s", nextCommand, commandParam);

        if (strcmp(nextCommand,"AUTHENTICATE")==0)
        {
            printf("Authenticating...\r\n");
            if (!Authenticate(usersFile, newSock, &user, commandParam, SMALL_BUFFER_SIZE))
            {
                strcpy(buffer, FAIL_MSG);
                // no need to check sendall since user failed to authenticate anyway.
                sendall(newSock, (char *)&buffer);
                close(newSock);
                continue;
            }
            printf("Authenticated successfully...\r\n");

            // sendall authentication successful message to client
            strcpy(buffer, SUCCESS_MSG);
            if (sendall(newSock, (char *)&buffer) == -1)
            {
                close(newSock);
                continue;
            }
        }
        else if (strcmp(nextCommand,"SHOW_INBOX")==0)
        {
            for (k = 1; k < j; k++)
            {
                i = active_user_emails[k];
                if (emails[i].active)
                {
                    sprintf(buffer, "%d. %s \"%s\"", k, emails[i].content->from, emails[i].content->title);
                    if (sendall(newSock, (char *)&buffer) == -1)
                    {
                        breakOuter = true;
                        break;
                    }
                }
            }
            if (breakOuter)
            {
                breakOuter = false;
                break;
            }
            sprintf(buffer, "END");

            if (sendall(newSock, (char *)&buffer) == -1)
            {
                break;
            }
        }
        else if (strcmp(nextCommand,"GET_MAIL")==0)
        {
            msg_id = get_msg_id(commandParam, active_user_emails);

            if (msg_id != -1 && emails[msg_id].active && strcmp(emails[msg_id].to, user) == 0)
            {
                sprintf(bigBuffer, "%s;%s;%s;%s", emails[msg_id].content->from, emails[msg_id].content->recipients_string,
                        emails[msg_id].content->title, emails[msg_id].content->text);
                if (sendall(newSock, (char *)&bigBuffer) == -1)
                {
                    break;
                }
            }
            else
            {
                strcpy(buffer, FAIL_MSG);
                if (sendall(newSock, (char *)&buffer) == -1)
                {
                    break;
                }
            }
        }
        else if (strcmp(nextCommand,"DELETE_MAIL")==0)
        {
            msg_id = get_msg_id(commandParam, active_user_emails);

            if (msg_id != -1 && strcmp(user, emails[msg_id].to) == 0)
            {
                emails[msg_id].active = false;
                strcpy(buffer, SUCCESS_MSG);
            }
            else
            {
                strcpy(buffer, FAIL_MSG);
            }

            if (sendall(newSock, (char *)&buffer) == -1)
            {
                break;
            }
        }
        else if (strcmp(nextCommand,"COMPOSE")==0)
        {
            if (curr_email >= MAXMAILS){
                strcpy(buffer, FAIL_MSG);
                if (sendall(newSock, (char *)&buffer) == -1)
                {
                    break;
                }
                continue;
            }

            sscanf(commandParam, "%[^;];%[^;];%[^;]", recipients_string, title, text);
            recipients = ExtractRecipients(recipients_string, &recipients_amount);

            emailContent = (EmailContent*)malloc(sizeof(EmailContent));

            strcpy(emailContent->from, user);
            strcpy(emailContent->title, title);
            strcpy(emailContent->text, text);
            strcpy(emailContent->recipients_string, recipients_string);
            emailContent->recipients = recipients;

            // foreach recipient create new email instance
            for (i = 0; i < recipients_amount; i++)
            {
                curr_email++;
                strcpy(emails[curr_email].to, recipients[i]);
                emails[curr_email].content = emailContent;
                emails[curr_email].active = true;

                if (strcmp(user, recipients[i]) == 0)
                {
                    active_user_emails[j] = curr_email;
                    j++;
                }
            }

            strcpy(buffer, SUCCESS_MSG);
            if (sendall(newSock, (char *)&buffer) == -1)
            {
                break;
            }
        }
    }
}

int init_listen(unsigned short portToListen)
{
    int errcheck;

    printf("creating socket...\r\n");
    int mainSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (mainSocket == -1)
    {
        printf("Error in function: socket()\r\n"
               "With error: %s\r\n", strerror(errno));
        return -1;
    }

    struct sockaddr_in my_addr;
    socklen_t addrlen = sizeof(my_addr);

    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    my_addr.sin_port = htons(portToListen);

    printf("binding...\r\n");
    errcheck = bind(mainSocket, (const struct sockaddr*)&my_addr, addrlen);
    if (errcheck == -1)
    {
        printf("Error in function: bind()\r\n"
               "With error: %s\r\n", strerror(errno));
        return -1;
    }

    printf("listening...\r\n");
    errcheck = listen(mainSocket, BACKLOG);
    if (errcheck == -1)
    {
        printf("Error in function: listen()\r\n"
               "With error: %s\r\n", strerror(errno));
        return -1;
    }

    return mainSocket;
}

bool Authenticate(char* usersFile, int socket, char** user, char* buffer, int bufferSize)
{
    char checkUsername[MAX_USERNAME];
    char checkPassword[MAX_PASSWORD];
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];

    sscanf(buffer, "%[^;];%[^;]", username, password);
    strcpy(*user, username);
    // read form file
    FILE* fp = fopen(usersFile, "r");

    while(fgets(buffer, bufferSize, fp) != NULL)
    {
        sscanf(buffer, "%s\t%s", checkUsername, checkPassword);
        if (strcmp(checkUsername, username) == 0 && strcmp(checkPassword, password) == 0)
        {
            return true;
        }
    }
    return false;

}

char** ExtractRecipients(char* recipients_string, int* amount)
{
    char** recipients = (char**)malloc(sizeof(char*)*TO_TOTAL);
    int i, j, cnt;
    char curr_recipient[MAX_USERNAME];
    cnt = 0;

    for (i = 0, j = 0; i < strlen(recipients_string); i++)
    {
        if (recipients_string[i] == ',')
        {
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

int get_msg_id(char* commandParam, int* active_user_emails)
{
    int user_msg_id;
    user_msg_id = atoi(commandParam);
    // handle invalid command parameter
    if (user_msg_id == 0 || user_msg_id > MAXMAILS)
    {
        return -1;
    }
    int msg_id = active_user_emails[user_msg_id];
    // handle invalid index
    if (msg_id == 0)
    {
        return -1;
    }
    return msg_id;
}

int getSocketByUser(char* user, Socket* sockets){
    int i;
    for (i=0;i<NUM_OF_CLIENTS;i++){
        if(sockets[i]->isActive && sockets[i]->isAuth){
            if (strcmp(user,socket[i]->user)==0){
                return sockets[i]->fd;
            }
        }
    }
    return -1;
}

int getSocketByfd(int fd, Socket* sockets){
    int i;
    for (i=0;i<NUM_OF_CLIENTS;i++){
        if(sockets[i]->fd == fd){
            return i;
        }
    }
    return -1;
}

void addNewSock(int fd, Socket* sockets){
    int i;
    for (i=0;i<NUM_OF_CLIENTS;i++){
         if (!sockets[i]->isActive){
            sockets[i]->isActive=true;
            sockets[i]->isAuth=false;
         }
    return -1;
}
void init_sockets(Socket* sockets){
    int i;
    for (i=0;i<NUM_OF_CLIENTS;i++){
        sockets[i]->isActive=false;
}
