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
#include "mail_client.h"

int main(int argc, char* argv[])
{
    if (argc > 3)
    {
        printf("Invalid input. please use the following format: \r\n"
               "mail_client <optional:hostname <optional:port>>\r\n");
        return -1;
    }
    char portToConnect[BUFFER_SIZE];
    char hostname[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];
    char user[MAX_USERNAME];
    char password[MAX_PASSWORD];
    char from[MAX_USERNAME*NUM_OF_CLIENTS];
    char to[NUM_OF_CLIENTS];
    char subject[MAX_SUBJECT];
    char content[MAX_CONTENT];
    char command[BUFFER_SIZE];
    bool breakOuter = false;
    int sock;

    if (argc==3)
    {
        strcpy(hostname, argv[1]);
        strcpy(portToConnect,argv[2]);
    }
    else if (argc==2)
    {
        sprintf(portToConnect,"%d", DEFAULT_PORT);
        strcpy(hostname, argv[1]);
    }
    else
    {
        sprintf(portToConnect,"%d", DEFAULT_PORT);
        strcpy(hostname, DEFAULT_HOST);
    }

    // Creating connection
    sock = create_connection(hostname,portToConnect);
    if (sock == -1)
    {
        // failed to create connection (error message already printed)
        return -1;
    }

    // Receiving greeting...
    if (recvall(sock, (char*)&buffer) == -1)
    {
        close(sock);
        return -1;
    }
    printf("%s\r\n",buffer);

    // Waiting for username and password
    scanf("User: %s", user);
    getchar(); // ignore \n char
    scanf("Password: %[^\n]s", password);
    getchar(); // ignore \n char

    sprintf(buffer, "AUTHENTICATE %s;%s", user, password);
    if (sendall(sock, (char *)&buffer) == -1)
    {
        close(sock);
        return -1;
    }

    // Waiting for server to react to authentication....
    if (recvall(sock, (char*)&buffer)==-1)
    {
        close(sock);
        return -1;
    }
    handleChatMessage(buffer, sock);

    if (strcmp(buffer,FAIL_MSG)==0)
    {
        printf("Authentication failed....\r\n Exiting....\r\n");
        close(sock);
        return -1;
    }

    printf("Connected to server\r\n");

    fd_set read_fds;

    while(true)
    {
        FD_ZERO(&read_fds);
        FD_SET(sock, &read_fds);
        FD_SET(STDIN, &read_fds);
        if (select(sock+1, &read_fds, NULL, NULL, NULL) == -1)
        {
            printf("Error in select(): %s", strerror(errno));
            break;
        }
        // if got message from server
        if (FD_ISSET(sock, &read_fds))
        {
            if (recvall(sock, (char*)&buffer)==-1)
            {
                break;
            }
            sscanf(buffer,"MSG %[^;];%[^\n]", from, content);
            printf("New message from %s: %s\n", from, content);
        }
        // if user inserted something
        if (FD_ISSET(STDIN, &read_fds))
        {
            scanf("%[^\n]s",buffer);
            getchar();
            sscanf(buffer, "%s",command); // get first word of command string (i.e COMPOSE, GET_MAIL)

            if (strcmp(command,"SHOW_INBOX")==0)
            {
                if (sendall(sock, (char *)&buffer) == -1)
                {
                    break;
                }
                if (recvall(sock, (char*)&buffer)==-1)
                {
                    break;
                }
                handleChatMessage(buffer, sock);
                while (strcmp(buffer,"END")!=0)
                {
                    printf("%s\n", buffer);
                    if (recvall(sock, (char*)&buffer) == -1)
                    {
                        breakOuter=true;
                        break;
                    }
                    handleChatMessage(buffer, sock);
                }
                if (breakOuter==true)
                {
                    break;
                }
            }
            else if (strcmp(command,"GET_MAIL")==0)
            {
                if (sendall(sock, (char *)&buffer) == -1)
                {
                    break;
                }
                if (recvall(sock, (char*)&buffer)==-1)
                {
                    break;
                }
                handleChatMessage(buffer, sock);
                if (strcmp(buffer,FAIL_MSG)==0)
                {
                    printf("oops, can't find the mail you requested...\r\n");
                }
                else
                {
                    // parsing buffer so it will be printed nicely
                    sscanf(buffer,"%[^;];%[^;];%[^;];%[^;]",from,to,subject,content);
                    printf("From: %s\r\nTo: %s\r\nSubject: %s\r\nText: %s\r\n",from,to,subject,content);
                }
            }
            else if (strcmp(command,"DELETE_MAIL") == 0)
            {
                if (sendall(sock, (char *)&buffer) == -1)
                {
                    break;
                }
                if (recvall(sock, (char*)&buffer) == -1)
                {
                    break;
                }
                handleChatMessage(buffer, sock);
                if (strcmp(buffer,FAIL_MSG) == 0)
                {
                    printf("oops, can't delete the mail you requested...\r\n");
                }
            }
            else if (strcmp(command,"COMPOSE") == 0)
            {
                // preparing buffer to send
                scanf("To: %[^\n]s", to);
                getchar(); // ignore \n char
                scanf("Subject: %[^\n]s", subject);
                getchar(); // ignore \n char
                scanf("Text: %[^\n]s", content);
                getchar(); // ignore \n char
                sprintf(buffer,"COMPOSE %s;%s;%s", to,subject,content);

                if (sendall(sock, (char *)&buffer)==-1)
                {
                    break;
                }
                if (recvall(sock, (char*)&buffer)==-1)
                {
                    break;
                }
                handleChatMessage(buffer, sock);
                if (strcmp(buffer,FAIL_MSG)==0)
                {
                    printf("Error in compose... \r\n");
                }
                else
                {
                    printf("Mail sent\r\n");
                }
            }
            else if (strcmp(command, "SHOW_ONLINE_USERS")==0)
            {
                if (sendall(sock, (char *)&buffer) == -1)
                {
                    break;
                }
                if (recvall(sock, (char*)&buffer) == -1)
                {
                    break;
                }
                handleChatMessage(buffer, sock);
                printf("Online users: %s\n", buffer);
            }
            else if (strcmp(command, "MSG")==0)
            {
                if (sendall(sock, (char *)&buffer)==-1)
                {
                    break;
                }
                if (recvall(sock, (char*)&buffer) == -1)
                {
                    break;
                }
                handleChatMessage(buffer, sock);
                if (strcmp(buffer,FAIL_MSG)==0)
                {
                    printf("Error in sending message to user... \r\n");
                }
                else
                {
                    printf("Message sent\r\n");
                }
            }
            else if (strcmp(command,"QUIT")==0)
            {
                // no need to check sendall() return value since loop breaks anyway.
                sendall(sock, (char *)&buffer);
                break;
            }
            else
            {
                printf("Command not supported.\r\n");
            }
        }
    }
    // closing connection
    safe_shutdown(sock, buffer);
}

int create_connection(char* hostname, char* portToConnect)
{
    int errcheck;
    int sock;
    struct addrinfo *serverinfo, *p;

    // creating socket...
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
    {
        printf("Could not create socket: %s\r\n", strerror(errno));
        return -1;
    }

    // getting address info....
    errcheck = getaddrinfo(hostname, portToConnect, NULL, &serverinfo);
    if (errcheck < 0)
    {
        printf("getaddrinfo() failed: %s\r\n", strerror(errno));
        close(sock);
        return -1;
    }

    // connecting...
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

void handleChatMessage(char* buffer, int sock)
{
    char msg[BUFFER_SIZE];
    char command[BUFFER_SIZE];
    char user[BUFFER_SIZE];

    sscanf(buffer, "%s", command);

    while (strcmp(command, "MSG") == 0)
    {
        sscanf(buffer,"MSG %[^;];%[^\n]", user, msg);
        printf("New message from %s: %s\n", user, msg);
        if (recvall(sock, buffer) == -1)
        {
            close(sock);
            exit(-1);
        }
        sscanf(buffer, "%s", command);
    }
}

