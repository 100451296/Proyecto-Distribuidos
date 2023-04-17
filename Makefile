CC = gcc
CFLAGS = -Wall -Werror

all: server

server: server.o manage.o
	$(CC) $(CFLAGS) -o server server.o manage.o

server.o: server.c manage.h
	$(CC) $(CFLAGS) -c server.c

manage.o: manage.c manage.h
	$(CC) $(CFLAGS) -c manage.c

clean:
	rm -f *.o server
