#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUF_SIZE 4096

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <IP> <meta 0|1> <token>\n", argv[0]);
        return 1;
    }

    char *ip = argv[1];
    char *meta = argv[2];
    char *token = argv[3];

    int sock;
    struct sockaddr_in server_addr;
    char request[1024];
    char buffer[BUF_SIZE];

    /* socket */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket error");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    /* connect */
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        return 1;
    }

    /* HTTP request */
    snprintf(request, sizeof(request),
        "GET /index.html?meta=%s HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Authorization: %s\r\n\r\n",
        meta, ip, PORT, token);

    /* send */
    send(sock, request, strlen(request), 0);

    /* receive */
    int bytes;
    while ((bytes = recv(sock, buffer, sizeof(buffer)-1, 0)) > 0) {
        buffer[bytes] = '\0';
        printf("%s", buffer);
    }

    close(sock);
    return 0;
}

