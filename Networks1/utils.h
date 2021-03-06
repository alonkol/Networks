#define BACKLOG 10
#define DEFAULT_HOST "localhost"
#define DEFAULT_PORT 6423
#define SUCCESS_MSG "Success"
#define FAIL_MSG "Failure"
#define NUM_OF_CLIENTS 20
#define MAX_USERNAME 50
#define MAX_PASSWORD 50
#define MAX_SUBJECT 100
#define MAX_CONTENT 2000
#define MAXMAILS 32000
#define TO_TOTAL 20
#define BUFFER_SIZE 5000
#define GREETING "With fire and blood"
#define NEW_MSG_TITLE "Message received offline"
#define STDIN 0

int sendall(int s, char *buf);
int recvall(int s, char *buf);
void safe_shutdown(int socket, char* buffer);
