#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>

#define PORT 9090
#define BUFFER_SIZE 4096

void write_log(const char *msg) {
    FILE *log = fopen("server.log", "a");
    if (log) {
        fprintf(log, "%s\n", msg);
        fclose(log);
    }
}

void daemonize() {
    pid_t pid = fork();

    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS); // родитель умирает

    setsid(); // новая сессия

    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    umask(0);
    chdir("/");

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE] = {0};

    int bytes = read(client_socket, buffer, BUFFER_SIZE - 1);
    if (bytes <= 0) {
        close(client_socket);
        return;
    }

    write_log(buffer);

    char login[100] = {0};
    char command[1024] = {0};

    sscanf(buffer, "{\"login\":\"%[^\"]\",\"command\":\"%[^\"]\"}", login, command);

    FILE *fp = popen(command, "r");
    if (!fp) {
        char *err = "{\"code\":1,\"result\":\"command error\"}";
        send(client_socket, err, strlen(err), 0);
        close(client_socket);
        return;
    }

    char result[BUFFER_SIZE] = {0};
    char temp[256];

    while (fgets(temp, sizeof(temp), fp)) {
        strcat(result, temp);
    }

    pclose(fp);

    char response[BUFFER_SIZE];
    snprintf(response, sizeof(response),
             "{\"code\":0,\"result\":\"%s\"}", result);

    send(client_socket, response, strlen(response), 0);

    close(client_socket);
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    daemonize(); // ← ВКЛЮЧАЕМ ДЕМОН

    write_log("Server started");

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 5);

    while (1) {
        client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);

        write_log("New connection");

        handle_client(client_socket);
    }

    return 0;
}
