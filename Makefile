CC=gcc

all: server client http

server:
	$(CC) server/server.c -o server/server

client:
	$(CC) client/client.c -o client/client

http:
	$(CC) server/http_server.c -o server/http_server

clean:
	rm -f server/server client/client server/http_server
