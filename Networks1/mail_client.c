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

#define DEFAULT_PORT "6423"
#define DEFAULT_HOST "localhost"
#define SUCCESS_MSG "Success"

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
            printf("Error in function sendall()\r\n"
                           "%s", strerror(errno));
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
            printf("Error in function recvall()\r\n"
                           "%s", strerror(errno));
            break;
        }
        if (n == 0) // client disconnected
        {
            printf("Client disconnected\r\n");
            return -1;
        }
        total += n;
        bytesleft -= n;
    }
    *len = total; /* return number actually sent here */
    return n == -1 ? -1:0; /*-1 on failure, 0 on success */
}

int create_connection(const char* hostname, const char* portToConnect)
{
    int errcheck;
    int sock;
    int port;
    struct addrinfo *serverinfo, *p;

    sscanf(portToConnect,"%d",&port);
    port = htons(port);
    sprintf(portToConnect,"%d",port);

    printf("creating socket...\r\n");
	sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
    {
        printf("Could not create socket: %s\n" , strerror(errno));
        return -1;
    }

	printf("getting address info\r\n");
    errcheck = getaddrinfo(hostname, portToConnect, NULL, &serverinfo);
	if (errcheck < 0) {
		printf("getaddrinfo() failed: %s\n", strerror(errno));
		close(sock);
		return -1;
	}

	printf("connecting...\r\n");
	// loop through all the results and connect to the first we can
	for(p = serverinfo; p != NULL; p = p->ai_next) {
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
    if (argc < 2 || argc > 4 || strcmp(argv[1],"mail_client")!=0)
    {
        printf("Invalid input. please use the following format: \r\n"
                       "mail_client <optional:hostname <optional:port>>\r\n");
        return -1;
    }
    char portToConnect[1024];
    char hostname[1024];
    int sock;

    if (argc==4)
    {
        strcpy(hostname, argv[2]);
        strcpy(portToConnect,argv[3]);
    }
    else if (argc==3)
    {
        strcpy(portToConnect, DEFAULT_PORT);
        strcpy(hostname, argv[2]);
    }
    else
    {
        strcpy(portToConnect,DEFAULT_PORT);
        strcpy(hostname, DEFAULT_HOST);
    }

    printf("Creating connection\r\n");
    sock = create_connection(hostname,portToConnect);
    if (sock == -1){
        // it means we failed to create connection (error message already printed)
        return 1;
    }

    printf("Receiving greeting...\r\n");

    char buffer[1024];
    int buffer_size = sizeof(buffer);
    char bigBuffer[10*1024];

    recvall(sock, (char*)&buffer, &buffer_size);
    printf("greeting: %s\r\n",buffer);

    char user[1024];
    char password[1024];

    printf("Waiting for username and password\r\n");
    scanf("User: %[^\n]s \r\n", user);
    scanf("Password: %[^\n]s \r\n", password);
    printf("%s %s\n", user, password);

    printf("sending username and password...\r\n");
    sprintf(buffer, "%s;%s", user, password);
    printf("%s", buffer);
    sendall(sock, (char *)&buffer, &buffer_size);

    printf("Waiting for server to react....\r\n");
    recvall(sock, (char*)&buffer, &buffer_size);

    if (strcmp(buffer,SUCCESS_MSG)!=0)
    {
        printf("Connection failed....\r\n Exiting....\r\n");
        close(sock);
        return 1;
    }
    printf("Connection established....\r\n");

    char from[1024];
    char to[1024];
    char subject[1024];
    char content[1024];
    char command[1024];

    while(true)
    {
        printf("Enter Command:\r\n");
        scanf("%[^\n]s",buffer);
        printf("sending command: %s\r\n",buffer);
        sendall(sock, (char *)&buffer, &buffer_size);
        sscanf(buffer, "%s",command);
        printf("%s",command);

        if (strcmp(command,"SHOW_INBOX")==0)
        {
            printf("IN SHOWINBOX\n");
            recvall(sock, (char*)&buffer, &buffer_size);
            while (strcmp(buffer,"END")!=0)
            {
                //TODO write msg nicely like example
                printf("%s\n", buffer);
                recvall(sock, (char*)&buffer, &buffer_size);
            }
            printf("END OF SHOW INBOX\n");
        }
        else if (strcmp(command,"GET_MAIL")==0)
        {
            printf("IN GETMAIL\n");
            int bigbuffersize= sizeof(bigBuffer);
            recvall(sock, (char*)&bigBuffer, &bigbuffersize);
            printf("got buff: %s\n",bigBuffer);
            sscanf(bigBuffer,"%[^\n]s ; %[^\n]s ; %[^\n]s ; %[^\n]s",from,to,subject,content);
            printf("From: %s\nTo: %s\nSubject: %s\nText: %s\n",from,to,subject,content);
        }
        else if (strcmp(command,"DELETE_MAIL")==0)
        {
            printf("IN DELETE\n");
            recvall(sock, (char*)&buffer, &buffer_size);
            printf("%s",buffer);
        }
        else if (strcmp(command,"COMPOSE")==0)
        {
            printf("IN COMPOSE\n");
            scanf("To: %[^\n]s", to);
            scanf("Subject: %[^\n]s", subject);
            scanf("Text: %[^\n]s", content);
            sprintf(buffer,"%s ; %s ; %s", to,subject,content);
            printf("sending: %s",buffer);
            sendall(sock, (char *)&buffer, &buffer_size);
            recvall(sock, (char*)&buffer, &buffer_size);
            printf("recieved: %s",buffer);
            //if sucsseded
            printf("Message sent..\r\n");
        }
        else if (strcmp(command,"QUIT")==0)
        {
            printf("IN QUIT");
            break;
        }else{
            printf("Command not supported.\r\n");
        }
    }

    close(sock);
}


