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
    char buffer[SMALL_BUFFER_SIZE];
    char nextCommand[SMALL_BUFFER_SIZE], commandParam[SMALL_BUFFER_SIZE];
    Email emails[MAXMAILS+1];
    int curr_email = 1, recipients_amount = 0;
    int i, k, msg_id;
    bool breakOuter = false;

    char recipients_string[(MAX_USERNAME+1)*TO_TOTAL];
    char title[MAX_SUBJECT];
    char text[MAX_CONTENT];

    char** recipients;
    EmailContent* emailContent;
    Sockets sockets[NUM_OF_CLIENTS];

    fd_set read_fds;
    int fdmax = 0;

    while(true)
    {
        FD_ZERO(&read_fds);
        FD_SET(mainSocket, &read_fds);
        for (i = 0; i < NUM_OF_CLIENTS; i++)
        {
            if (sockets[i]->isActive)
            {
                FD_SET(sockets[i]->fd, &read_fds);
            }
        }
        fdmax = getMaxFd(sockets);
        fdmax = fdmax > mainSocket ? fdmax : mainSocket;
        select(fdmax+1, &read_fds, NULL, NULL, NULL);

        if (FD_ISSET(listening_sock , &read_fds))
        {
            printf("accepting...\r\n");
            newSock = accept(mainSocket, NULL, &addrlen);
            i = AddNewSocket(newSock);
            printf("Accepted new client\r\n");
            strcpy(buffer, GREETING);
            // Send greeting
            if (sendall(sockets[i]->fd, (char *)&buffer) == -1)
            {
                sockets[i]->isActive = false;
                continue;
            }
        }

        for (i = 0; i < NUM_OF_CLIENTS; i++)
        {
            if (FD_ISSET(sockets[i]->fd , &read_fds))
            {
                // accept commands
                if (recvall(sockets[i]->fd, (char*)&buffer) == -1)
                {
                    socket[i]->isActive = false;
                    continue;
                }
                sscanf(buffer, "%s %s", nextCommand, commandParam);

                // if user not authenticated, next command must be AUTHENTICATE
                if (!sockets[i]->isAuth && strcmp(nextCommand,"AUTHENTICATE")!=0)
                {
                    strcpy(buffer, FAIL_MSG);
                    if (sendall(sockets[i]->fd, (char *)&buffer) == -1)
                    {
                        socket[i]->isActive = false;
                        continue;
                    }
                }

                if (strcmp(nextCommand,"AUTHENTICATE")==0)
                {
                    printf("Authenticating...\r\n");
                    if (!Authenticate(usersFile, socket[i], commandParam, SMALL_BUFFER_SIZE))
                    {
                        strcpy(buffer, FAIL_MSG);
                        if (sendall(sockets[i]->fd, (char *)&buffer) == -1)
                        {
                            socket[i]->isActive = false;
                        }
                        continue;
                    }

                    printf("Authenticated successfully...\r\n");
                    strcpy(buffer, SUCCESS_MSG);
                }
                else if (strcmp(nextCommand,"SHOW_INBOX")==0)
                {
                    for (k = 1; k < j; k++)
                    {
                        i = sockets[i]->active_user_emails[k];
                        if (emails[i].active)
                        {
                            sprintf(buffer, "%d. %s \"%s\"", k, emails[i].content->from, emails[i].content->title);
                            if (sendall(sockets[i]->fd, (char *)&buffer) == -1)
                            {
                                socket[i]->isActive = false;
                                continue;
                            }
                        }
                    }

                    sprintf(buffer, "END");
                }
                else if (strcmp(nextCommand,"GET_MAIL")==0)
                {
                    msg_id = get_msg_id(commandParam, sockets[i]->active_user_emails);

                    if (msg_id != -1 && emails[msg_id].active && strcmp(emails[msg_id].to, sockets[i]->user) == 0)
                    {
                        sprintf(buffer, "%s;%s;%s;%s", emails[msg_id].content->from, emails[msg_id].content->recipients_string,
                                emails[msg_id].content->title, emails[msg_id].content->text);
                    }
                    else
                    {
                        strcpy(buffer, FAIL_MSG);
                    }
                }
                else if (strcmp(nextCommand,"DELETE_MAIL")==0)
                {
                    msg_id = get_msg_id(commandParam, sockets[i]->active_user_emails);

                    if (msg_id != -1 && strcmp(user, emails[msg_id].to) == 0)
                    {
                        emails[msg_id].active = false;
                        strcpy(buffer, SUCCESS_MSG);
                    }
                    else
                    {
                        strcpy(buffer, FAIL_MSG);
                    }
                }
                else if (strcmp(nextCommand,"COMPOSE")==0)
                {
                    if (curr_email >= MAXMAILS){
                        strcpy(buffer, FAIL_MSG);
                        if (sendall(sockets[i]->fd, (char *)&buffer) == -1)
                        {
                            socket[i]->isActive = false;
                        }
                        continue;
                    }

                    sscanf(commandParam, "%[^;];%[^;];%[^;]", recipients_string, title, text);
                    recipients = ExtractRecipients(recipients_string, &recipients_amount);

                    emailContent = (EmailContent*)malloc(sizeof(EmailContent));

                    strcpy(emailContent->from, sockets[i]->user);
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

                        if (strcmp(sockets[i]->user, recipients[i]) == 0)
                        {
                            sockets[i]->active_user_emails[j] = curr_email;
                            j++;
                        }
                    }

                    strcpy(buffer, SUCCESS_MSG);
                }
                else if (strcmp(nextCommand,"QUIT")==0)
                {
                    socket[i]->isActive = false;
                    continue;
                }
                else
                {
                    strcpy(buffer, FAIL_MSG);
                }

                // Send response
                if (sendall(sockets[i]->fd, (char *)&buffer) == -1)
                {
                    socket[i]->isActive = false;
                }
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

bool Authenticate(char* usersFile, Socket socket, char* buffer, int bufferSize)
{
    char checkUsername[MAX_USERNAME];
    char checkPassword[MAX_PASSWORD];
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];

    sscanf(buffer, "%[^;];%[^;]", username, password);
    // read form file
    FILE* fp = fopen(usersFile, "r");

    while(fgets(buffer, bufferSize, fp) != NULL)
    {
        sscanf(buffer, "%s\t%s", checkUsername, checkPassword);
        if (strcmp(checkUsername, username) == 0 && strcmp(checkPassword, password) == 0)
        {
            // authenticated successfully
            socket->isAuth = true;
            strcpy(socket->user, username);

            // load user emails to memory
            int j = 1;
            socket->active_user_emails = {0};
            for (i = 1; i <= curr_email; i++)
            {
                if (strcmp(emails[i].to, username) == 0)
                {
                    socket->active_user_emails[j] = i;
                    j++;
                }
            }

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

int getSocketByFd(int fd, Socket* sockets){
    int i;
    for (i=0;i<NUM_OF_CLIENTS;i++){
        if(sockets[i]->fd == fd){
            return i;
        }
    }
    return -1;
}

int addNewSock(int fd, Socket* sockets){
    int i;
    for (i=0;i<NUM_OF_CLIENTS;i++){
         if (!sockets[i]->isActive){
            sockets[i]->isActive=true;
            sockets[i]->isAuth=false;
            return i;
         }
    return -1;
}
void init_sockets(Socket* sockets){
    int i;
    for (i=0;i<NUM_OF_CLIENTS;i++){
        sockets[i]->isActive=false;
    }
}

int getMaxFd(Socket* sockets){
    int i,max=-1;
    for (i=0;i<NUM_OF_CLIENTS;i++){
        if (sockets[i].isActive && max<sockets[i]->fd){
            max=sockets[i]->fd;
        }
    }
    return max;
}
