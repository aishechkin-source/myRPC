CC=gcc

all: server client

server:
	$(CC) server/server.c -o server/server

client:
	$(CC) client/client.c -o client/client

clean:
	rm -f server/server client/client
