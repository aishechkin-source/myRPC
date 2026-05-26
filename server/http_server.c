#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#define BUF_SIZE 4096

/* ===== DEMON ===== */
void daemonize() {
    pid_t pid = fork();

    if (pid < 0)
        exit(EXIT_FAILURE);

    if (pid > 0)
        exit(EXIT_SUCCESS);

    umask(0);

    if (setsid() < 0)
        exit(EXIT_FAILURE);

    if (chdir("/") < 0)
        exit(EXIT_FAILURE);

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

/* ===== SIGNAL ===== */
void handle_signal(int sig) {
    syslog(LOG_INFO, "Shutting down...");
    closelog();
    exit(0);
}

/* ===== IP WHITELIST ===== */
int is_ip_allowed(const char *ip) {

    FILE *file = fopen("/etc/myRPC/Whitelist.conf", "r");

    if (!file)
        return 0;

    char line[128];

    while (fgets(line, sizeof(line), file)) {

        line[strcspn(line, "\n")] = 0;

        if (strcmp(line, ip) == 0) {
            fclose(file);
            return 1;
        }
    }

    fclose(file);

    return 0;
}

/* ===== TOKEN ===== */
int check_token(char *req) {
    return strstr(req, "Authorization: Ishechkin0722") != NULL;
}

/* ===== META ===== */
int get_meta(char *req) {

    if (strstr(req, "meta=1"))
        return 1;

    return 0;
}

/* ===== CONFIG PORT ===== */
int load_port() {

    FILE *f = fopen("/etc/myRPC/myRPC.conf", "r");

    if (!f)
        return 8080;

    char line[128];
    int port = 8080;

    while (fgets(line, sizeof(line), f)) {

        if (sscanf(line, "port = %d", &port) == 1) {
            break;
        }
    }

    fclose(f);

    return port;
}

/* ===== CLIENT ===== */
void handle_client(int client_fd, char *client_ip) {

    char buffer[BUF_SIZE] = {0};

    recv(client_fd, buffer, sizeof(buffer), 0);

    syslog(LOG_INFO, "Request received from %s", client_ip);

    /* ===== IP CHECK ===== */
    if (!is_ip_allowed(client_ip)) {

        send(client_fd,
             "HTTP/1.1 403 Forbidden\r\n\r\nUser not allowed",
             49,
             0);

        close(client_fd);

        return;
    }

    /* ===== TOKEN CHECK ===== */
    if (!check_token(buffer)) {

        char *err =
            "HTTP/1.1 403 Forbidden\r\n"
            "Content-Type: text/plain\r\n\r\n"
            "Invalid token";

        send(client_fd, err, strlen(err), 0);

        close(client_fd);

        return;
    }

    int meta = get_meta(buffer);

    /* ===== TMP LOG ===== */
    FILE *tmp = fopen("/tmp/myRPC_log.txt", "a");

    if (tmp) {
        fprintf(tmp, "IP=%s meta=%d\n", client_ip, meta);
        fclose(tmp);
    }

    syslog(LOG_INFO,
           "Client %s handled with meta=%d",
           client_ip,
           meta);

    char response[2048];

    if (meta == 0) {

        snprintf(response,
                 sizeof(response),

                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: text/html\r\n\r\n"

                 "<html>"
                 "<body>"
                 "<h1>Meta 0: Index page</h1>"
                 "</body>"
                 "</html>");

    } else {

        snprintf(response,
                 sizeof(response),

                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: text/html\r\n\r\n"

                 "<html>"
                 "<body>"
                 "<h1>Meta 1: Other page</h1>"
                 "</body>"
                 "</html>");
    }

    send(client_fd, response, strlen(response), 0);

    close(client_fd);
}

/* ===== MAIN ===== */
int main() {

    int server_fd, client_fd;

    struct sockaddr_in addr;

    signal(SIGTERM, handle_signal);
    signal(SIGINT, handle_signal);

    daemonize();

    openlog("myRPC", LOG_PID, LOG_DAEMON);

    syslog(LOG_INFO, "Server starting...");

    int port = load_port();

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0) {

        syslog(LOG_ERR, "Socket creation failed");

        exit(EXIT_FAILURE);
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_fd,
             (struct sockaddr*)&addr,
             sizeof(addr)) < 0) {

        syslog(LOG_ERR, "Bind failed");

        close(server_fd);

        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0) {

        syslog(LOG_ERR, "Listen failed");

        close(server_fd);

        exit(EXIT_FAILURE);
    }

    syslog(LOG_INFO,
           "HTTP server started on port %d",
           port);

    while (1) {

        client_fd = accept(server_fd, NULL, NULL);

        if (client_fd < 0) {

            syslog(LOG_ERR, "Accept failed");

            continue;
        }

        /* ===== CLIENT IP ===== */
        struct sockaddr_in client_addr;

        socklen_t addr_len = sizeof(client_addr);

        getpeername(client_fd,
                    (struct sockaddr*)&client_addr,
                    &addr_len);

        char *client_ip =
            inet_ntoa(client_addr.sin_addr);

        syslog(LOG_INFO,
               "Connection from %s",
               client_ip);

        if (fork() == 0) {

            close(server_fd);

            handle_client(client_fd, client_ip);

            exit(0);
        }

        close(client_fd);
    }

    closelog();

    return 0;
}
