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
#define TO_TOTAL 20
#define MAX_USERNAME 50
#define MAX_PASSWORD 50
#define MAX_SUBJECT 100
#define MAX_CONTENT 2000
#define GREETING "Winter is coming"
#define FAIL_MSG "Failure"
#define SUCCESS_MSG "Success"
#define MAXMAILS 32000
#define DEFAULT_PORT 6423
#define NUM_OF_CLIENTS 20
#define SMALL_BUFFER_SIZE 100
#define BIG_BUFFER_SIZE 5000

int sendall(int s, char *buf);
int recvall(int s, char *buf);
int init_listen(unsigned short portToListen);
bool Authenticate(char* usersFile, int socket, char** user);
char** ExtractRecipients(char* recipients_string, int* amount);
int get_msg_id(char* commandParam, int* active_user_emails);

typedef struct email_content{

    char** recipients;
    char from[MAX_USERNAME];
    char recipients_string[(MAX_USERNAME+1)*TO_TOTAL];
    char title[MAX_SUBJECT];
    char text[MAX_CONTENT];
} EmailContent;

typedef struct email{
    char to[MAX_USERNAME];
    EmailContent* content;
    bool active;
} Email;

int main(int argc, char* argv[]) {
    if (argc !=2 && argc != 3){
        printf("Invalid input. please use the following format: \r\n"
                       "mail_server <users file> <optional:port>\r\n");
        return -1;
    }

    unsigned short portToListen = DEFAULT_PORT;

    char* usersFile = argv[1];
    if (argc == 3){
        portToListen = (u_short) strtoul(argv[2], NULL, 0);
    }

    struct sockaddr_in my_addr;
    socklen_t addrlen = sizeof(my_addr);
    int mainSocket = init_listen(portToListen);
    if (mainSocket == -1){
        return -1;
    }

    int errcheck;
    int newSock;
    char buffer[SMALL_BUFFER_SIZE], bigBuffer[BIG_BUFFER_SIZE];
    char nextCommand[SMALL_BUFFER_SIZE], commandParam[SMALL_BUFFER_SIZE];
    char* user = (char*)malloc(MAX_USERNAME);
    Email emails[MAXMAILS+1];
    int curr_email = 1, recipients_amount = 0;
    int i, k, msg_id, user_msg_id;
    bool breakOuter = false;

    char recipients_string[(MAX_USERNAME+1)*TO_TOTAL];
    char title[MAX_SUBJECT];
    char text[MAX_CONTENT];

    char** recipients;
    EmailContent* emailContent;

    while(true){
        printf("accepting...\r\n");
        newSock = accept(mainSocket, NULL, &addrlen);
        printf("Accepted new client\r\n");

        strcpy(buffer, GREETING);
        errcheck = sendall(newSock, (char *)&buffer);
        if (errcheck == -1)
        {
            close(newSock);
            continue;
        }

        printf("Authenticating...\r\n");
        if (!Authenticate(usersFile, newSock, &user)){
            strcpy(buffer, FAIL_MSG);
            // no need to check sendall since user failed to authenticate anyway.
            sendall(newSock, (char *)&buffer);
            close(newSock);
            continue;
        }
        printf("Authenticated successfully...\r\n");

        // sendall authentication successful message to client
        strcpy(buffer, SUCCESS_MSG);
        errcheck = sendall(newSock, (char *)&buffer);
        if (errcheck == -1)
        {
            close(newSock);
            continue;
        }

        // connected, load user emails to memory
        int j = 1;
        int active_user_emails[MAXMAILS+1] = {0};
        for (i = 1; i <= curr_email; i++){
            if (strcmp(emails[i].to, user) == 0){
                active_user_emails[j] = i;
                j++;
            }
        }

        // accept commands
        printf("Waiting for command...\r\n");
        errcheck = recvall(newSock, (char*)&buffer);
        if (errcheck == -1)
        {
            close(newSock);
            continue;
        }
        sscanf(buffer, "%s %s", nextCommand, commandParam);
        while(strcmp(nextCommand,"QUIT") != 0){
            if (strcmp(nextCommand,"SHOW_INBOX")==0){
                for (k = 1; k < j; k++){
                    i = active_user_emails[k];
                    if (emails[i].active){
                        sprintf(buffer, "%d. %s \"%s\"", k, emails[i].content->from, emails[i].content->title);
                        errcheck = sendall(newSock, (char *)&buffer);
                        if (errcheck == -1)
                        {
                            breakOuter = true;
                            break;
                        }
                    }
                }
                if (breakOuter) {
                    breakOuter = false;
                    break;
                }
                sprintf(buffer, "END");
                errcheck = sendall(newSock, (char *)&buffer);
                if (errcheck == -1)
                {
                    break;
                }
            } else if (strcmp(nextCommand,"GET_MAIL")==0){
                msg_id = get_msg_id(commandParam, active_user_emails);
                if (msg_id == -1){
                    strcpy(buffer, FAIL_MSG);
                    errcheck = sendall(newSock, (char *)&buffer);
                    break;
                }

                if (emails[msg_id].active && strcmp(emails[msg_id].to, user) == 0){
                    sprintf(bigBuffer, "%s;%s;%s;%s", emails[msg_id].content->from, emails[msg_id].content->recipients_string,
                            emails[msg_id].content->title, emails[msg_id].content->text);
                    int bigbuffersize = BIG_BUFFER_SIZE;
                    errcheck = sendall(newSock, (char *)&bigBuffer);
                    if (errcheck == -1)
                    {
                        break;
                    }
                } else {
                    strcpy(buffer, FAIL_MSG);
                    errcheck = sendall(newSock, (char *)&buffer);
                    if (errcheck == -1)
                    {
                        break;
                    }
                }
            } else if (strcmp(nextCommand,"DELETE_MAIL")==0){
                msg_id = get_msg_id(commandParam, active_user_emails);
                if (msg_id == -1){
                    strcpy(buffer, FAIL_MSG);
                    errcheck = sendall(newSock, (char *)&buffer);
                    break;
                }

                if (strcmp(user, emails[msg_id].to) == 0){
                    emails[msg_id].active = false;
                    strcpy(buffer, SUCCESS_MSG);
                } else {
                    strcpy(buffer, FAIL_MSG);
                }
                errcheck = sendall(newSock, (char *)&buffer);
                if (errcheck == -1)
                {
                    break;
                }
            } else if (strcmp(nextCommand,"COMPOSE")==0){
                errcheck = recvall(newSock, (char*)&buffer);
                if (errcheck == -1)
                {
                    break;
                }

                sscanf(buffer, "%[^;];%[^;];%[^;]", recipients_string, title, text);
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

                    if (strcmp(user, recipients[i]) == 0){
                        active_user_emails[j] = curr_email;
                        j++;
                    }
                }

                strcpy(buffer, SUCCESS_MSG);
                errcheck = sendall(newSock, (char *)&buffer);
                if (errcheck == -1)
                {
                    break;
                }
            }
            printf("Waiting for command...\r\n");
            errcheck = recvall(newSock, (char*)&buffer);
            if (errcheck == -1)
            {
                break;
            }
            sscanf(buffer, "%s %s", nextCommand, commandParam);
        }
        printf("Closing...\r\n");
        close(newSock);
    }
}

int init_listen(unsigned short portToListen){
    int errcheck;

    printf("creating socket...\r\n");
    int mainSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (mainSocket == -1){
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

    return mainSocket;
}

bool Authenticate(char* usersFile, int socket, char** user){
    char buffer[SMALL_BUFFER_SIZE];
    char checkUsername[MAX_USERNAME];
    char checkPassword[MAX_PASSWORD];
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];

    // receive authentication data from client
    if (recvall(socket, buffer) == -1)
    {
        return false;
    }

    sscanf(buffer, "%[^;];%[^;]", username, password);
    strcpy(*user, username);
    // read form file
    FILE* fp = fopen(usersFile, "r");

    while(fgets(buffer, SMALL_BUFFER_SIZE, fp) != NULL){
        sscanf(buffer, "%s\t%s", checkUsername, checkPassword);
        if (strcmp(checkUsername, username) == 0 && strcmp(checkPassword, password) == 0){
            return true;
        }
    }
    return false;

}

char** ExtractRecipients(char* recipients_string, int* amount){
    char** recipients = (char**)malloc(sizeof(char*)*TO_TOTAL);
    int i, j, cnt;
    char curr_recipient[MAX_USERNAME];
    cnt = 0;

    for (i = 0, j = 0; i < strlen(recipients_string); i++){
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

int get_msg_id(char* commandParam, int* active_user_emails){
    int user_msg_id;
    user_msg_id = atoi(commandParam);
    // handle invalid command parameter
    if (user_msg_id == 0 || user_msg_id > MAXMAILS){
        return -1;
    }
    int msg_id = active_user_emails[user_msg_id];
    // handle invalid index
    if (msg_id == 0){
        return -1;
    }

    return msg_id;
}


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
    if (n == -1){
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
    return n == -1 ? -1:0; /*-1 on failure, 0 on success */
}
