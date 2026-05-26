#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    char *host = "127.0.0.1";
    int port = 9090;
    char command[256] = {0};

    int opt;

    // getopt
    while ((opt = getopt(argc, argv, "h:p:c:")) != -1) {
        switch (opt) {
            case 'h':
                host = optarg;
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case 'c':
                strncpy(command, optarg, sizeof(command) - 1);
                break;
            default:
                printf("Использование: %s -h <host> -p <port> -c \"command\"\n", argv[0]);
                exit(1);
        }
    }

    if (strlen(command) == 0) {
        printf("Команда не указана!\n");
        exit(1);
    }

    int sock;
    struct sockaddr_in server_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, host, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        exit(1);
    }

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        exit(1);
    }

    printf("Подключено к серверу\n");

    // JSON запрос
    char request[BUFFER_SIZE];
    snprintf(request, sizeof(request),
             "{\"login\":\"alex\",\"command\":\"%s\"}", command);

    send(sock, request, strlen(request), 0);

    // ЧТЕНИЕ ОТВЕТА (🔥 исправлено)
    char response[BUFFER_SIZE];
    int total = 0;
    int bytes;

    while ((bytes = read(sock, response + total, BUFFER_SIZE - total - 1)) > 0) {
        total += bytes;
    }

    if (bytes < 0) {
        perror("read");
    }

    response[total] = '\0';

    printf("Ответ: %s\n", response);

    close(sock);

    return 0;
}
