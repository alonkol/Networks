#include <stdbool.h>

int init_listen(unsigned short portToListen);
bool Authenticate(char* usersFile, int socket, char** user, char* buffer, int bufferSize);
char** ExtractRecipients(char* recipients_string, int* amount);
int get_msg_id(char* commandParam, int* active_user_emails);
int getSocketByUser(char* user, Socket* sockets);
int getSocketByfd(int fd, Socket* sockets);
void addNewSock(int fd, Socket* sockets);
void init_sockets(Socket* sockets);

typedef struct email_content
{
    char** recipients;
    char from[MAX_USERNAME];
    char recipients_string[(MAX_USERNAME+1)*TO_TOTAL];
    char title[MAX_SUBJECT];
    char text[MAX_CONTENT];
} EmailContent;

typedef struct email
{
    char to[MAX_USERNAME];
    EmailContent* content;
    bool active;
} Email;

typedef struct socket
{
    int fd;
    char user[MAX_USERNAME];
    int[MAXMAILS+1] active_user_emails;
    bool isAuth;
    bool isActive;
}Socket;
