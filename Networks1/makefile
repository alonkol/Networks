CC = gcc -D_GNU_SOURCE
COMP_FLAG = -std=c99 -g -Wall 

all: mail_server mail_client

mail_server.o: mail_server.c mail_server.h utils.h
	$(CC) $(COMP_FLAG) -c mail_server.c
mail_client.o: mail_client.c mail_client.h utils.h
	$(CC) $(COMP_FLAG) -c mail_client.c
utils.o: utils.c utils.h
	$(CC) $(COMP_FLAG) -c utils.c

mail_server: mail_server.o utils.o
	$(CC) $(COMP_FLAG) mail_server.o utils.o -o mail_server
mail_client: mail_client.o utils.o
	$(CC) $(COMP_FLAG) mail_client.o utils.o -o mail_client

clean:
	rm -f *.o mail_client mail_server
