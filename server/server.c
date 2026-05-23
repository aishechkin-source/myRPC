#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    char buffer[1024] = {0};

    // создаём сокет
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // настройки адреса
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // привязка
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));

    // слушаем
    listen(server_fd, 3);

    printf("Сервер запущен на порту %d...\n", PORT);

    // принимаем клиента
    client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);

    // получаем сообщение
    recv(client_socket, buffer, 1024, 0);
    printf("Сообщение от клиента: %s\n", buffer);

    // обработка команды
    char command[10];
    int a, b;

    sscanf(buffer, "%s %d %d", command, &a, &b);

    int result = 0;

    if (strcmp(command, "add") == 0) {
        result = a + b;
    } else {
        char *msg = "Неизвестная команда\n";
        send(client_socket, msg, strlen(msg), 0);
        close(client_socket);
        close(server_fd);
        return 0;
    }

    // отправка результата
    char response[50];
    sprintf(response, "%d\n", result);
    send(client_socket, response, strlen(response), 0);

    // закрываем соединение
    close(client_socket);
    close(server_fd);

    return 0;
}
