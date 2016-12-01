#First for the server
CC = gcc
OBJS = mail_server.o utils.o
EXEC = mail_server
COMP_FLAG = -std=c99 -g -Wall -Wextra \
-Werror -pedantic-errors

$(EXEC): $(OBJS)
	$(CC) $(OBJS) -o $@
utils.o: utils.c utils.h
	$(CC) $(COMP_FLAG) -c $*.c
mail_server.o: mail_server.h mail_Server.c
	$(CC) $(COMP_FLAG) -c $*.c
clean:
	rm -f $(OBJS) $(EXEC)