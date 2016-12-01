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
#define FAIL_MSG "Failure"
#define MAX_USERNAME 50
#define MAX_PASSWORD 50
#define NUM_OF_CLIENTS 20
#define MAX_SUBJECT 100
#define MAX_CONTENT 2000
#define SMALL_BUFFER_SIZE 100
#define BIG_BUFFER_SIZE 5000

int sendall(int s, char *buf)
{
    int total = 0; /* how many bytes we've sent */
    int n;
    // first two bytes will be the message length
    int totalLen = strlen(buf);
    short* len_byte = (short*)&totalLen;
    sprintf(buf,"%i%s",*len_byte,buf);
    int bytesleft = totalLen+2;
    while(total < totalLen+2)
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

int create_connection(char* hostname, char* portToConnect)
{
    int errcheck;
    int sock;
    int port;
    struct addrinfo *serverinfo, *p;

    printf("creating socket...\r\n");
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
    {
        printf("Could not create socket: %s\n", strerror(errno));
        return -1;
    }

    printf("getting address info\r\n");
    errcheck = getaddrinfo(hostname, portToConnect, NULL, &serverinfo);
    if (errcheck < 0)
    {
        printf("getaddrinfo() failed: %s\n", strerror(errno));
        close(sock);
        return -1;
    }

    printf("connecting...\r\n");
    // loop through all the results and connect to the first we can
    for(p = serverinfo; p != NULL; p = p->ai_next)
    {
        errcheck = connect(sock, p->ai_addr, p->ai_addrlen);
        if (errcheck)
        {
            // try the next in list...
            continue;
        }
        break;
    }

    freeaddrinfo(serverinfo);
    // if failed - quit!
    if (errcheck)
    {
        printf("Error in function: connect()\r\n"
               "With error: %s\r\n", strerror(errno));
        close(sock);
        return -1;
    }

    return sock;
}

int main(int argc, char* argv[])
{
    if (argc > 3)
    {
        printf("Invalid input. please use the following format: \r\n"
               "mail_client <optional:hostname <optional:port>>\r\n");
        return -1;
    }
    char portToConnect[SMALL_BUFFER_SIZE];
    char hostname[SMALL_BUFFER_SIZE];
    int sock,errcheck;

    if (argc==3)
    {
        strcpy(hostname, argv[1]);
        strcpy(portToConnect,argv[2]);
    }
    else if (argc==2)
    {
        strcpy(portToConnect, DEFAULT_PORT);
        strcpy(hostname, argv[1]);
    }
    else
    {
        strcpy(portToConnect,DEFAULT_PORT);
        strcpy(hostname, DEFAULT_HOST);
    }

    printf("Creating connection\r\n");
    sock = create_connection(hostname,portToConnect);
    if (sock == -1)
    {
        // it means we failed to create connection (error message already printed)
        return 1;
    }

    printf("Receiving greeting...\r\n");

    char buffer[SMALL_BUFFER_SIZE];
    char bigBuffer[BIG_BUFFER_SIZE];

    errcheck = recvall(sock, (char*)&buffer);
    if (errcheck==-1)
    {
        return -1;
    }
    printf("greeting: %s\r\n",buffer);

    char user[MAX_USERNAME];
    char password[MAX_PASSWORD];

    printf("Waiting for username and password\r\n");
    scanf("User: %s", user);
    getchar();
    scanf("Password: %[^\n]s", password);
    getchar();

    sprintf(buffer, "%s;%s", user, password);
    sendall(sock, (char *)&buffer);

    printf("Waiting for server to react....\r\n");
    errcheck = recvall(sock, (char*)&buffer);
    if (errcheck==-1)
    {
        return -1;
    }

    if (strcmp(buffer,SUCCESS_MSG)!=0)
    {
        printf("Connection failed....\r\n Exiting....\r\n");
        close(sock);
        return -1;
    }
    printf("Connection established....\r\n");

    char from[MAX_USERNAME*NUM_OF_CLIENTS];
    char to[NUM_OF_CLIENTS];
    char subject[MAX_SUBJECT];
    char content[MAX_CONTENT];
    char command[SMALL_BUFFER_SIZE];

    while(true)
    {
        printf("Enter Command:\r\n");
        scanf("%[^\n]s",buffer);
        getchar();
        if ((errcheck = sendall(sock, (char *)&buffer))==-1)
        {
            break;
        }
        sscanf(buffer, "%s",command);

        if (strcmp(command,"SHOW_INBOX")==0)
        {
            errcheck = recvall(sock, (char*)&buffer);
            if (errcheck==-1)
            {
                break;
            }
            while (strcmp(buffer,"END")!=0)
            {
                printf("%s\n", buffer);
                errcheck = recvall(sock, (char*)&buffer);
                if (errcheck==-1)
                {
                    break;
                }
            }
        }
        else if (strcmp(command,"GET_MAIL")==0)
        {
            if ((errcheck=recvall(sock, (char*)&bigBuffer))==-1)
            {
                break;
            }
            if (strcmp(bigBuffer,FAIL_MSG)==0)
            {
                printf("oops, can't find the mail you requested...\r\n");
            }
            else
            {
                sscanf(bigBuffer,"%[^;];%[^;];%[^;];%[^;]",from,to,subject,content);
                printf("From: %s\nTo: %s\nSubject: %s\nText: %s\n",from,to,subject,content);
            }
        }
        else if (strcmp(command,"DELETE_MAIL")==0)
        {
            if ((errcheck= recvall(sock, (char*)&buffer))==-1)
            {
                break;
            }
            if (strcmp(buffer,FAIL_MSG)==0)
            {
                printf("oops, can't delete the mail you requested...\r\n");
            }
        }
        else if (strcmp(command,"COMPOSE")==0)
        {
            scanf("To: %[^\n]s", to);
            getchar();
            scanf("Subject: %[^\n]s", subject);
            getchar();
            scanf("Text: %[^\n]s", content);
            getchar();
            sprintf(buffer,"%s;%s;%s", to,subject,content);

            if ((errcheck=sendall(sock, (char *)&buffer))==-1)
            {
                break;
            }

            if ((errcheck = recvall(sock, (char*)&buffer))==-1)
            {
                break;
            }
            if (strcmp(buffer,SUCCESS_MSG)!=0)
            {
                printf("Error in compose...\r\n Exiting... \r\n");
            }
            else
            {
                printf("Message sent..\r\n");
            }
        }
        else if (strcmp(command,"QUIT")==0)
        {
            break;
        }
        else
        {
            printf("Command not supported.\r\n");
        }
    }
    close(sock);
}


