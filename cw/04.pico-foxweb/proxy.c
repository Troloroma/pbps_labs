#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sqli_filter.h"

#define PROXY_PORT 8080
#define BACKEND_ADDR "127.0.0.1"
#define BACKEND_PORT 8000
#define BUF_SIZE 65536

void handle_client(int client_fd) {
    char buf[BUF_SIZE];
    int n = recv(client_fd, buf, BUF_SIZE, 0);
    if (n <= 0) { close(client_fd); return; }
    buf[n] = '\0';

    char temp_buf[BUF_SIZE];
    memcpy(temp_buf, buf, n + 1);

    char *saveptr;
    char *line_end = strstr(temp_buf, "\r\n");
    if (!line_end) { close(client_fd); return; }
    *line_end = '\0';
    char *method = strtok_r(temp_buf, " \t", &saveptr);
    char *full_uri = strtok_r(NULL, " \t", &saveptr);
    *line_end = '\r';

    if (!full_uri) { close(client_fd); return; }

    // 2) отделяем query string
    char *uri = full_uri;
    char *qs = strchr(full_uri, '?');
    if (qs) {
        *qs++ = '\0';              // теперь qs — это всё после '?'
    } else {
        qs = "";                    // пустая строка, если нет '?'
    }

    // 3) находим тело запроса (если есть) — после CRLFCRLF
    char *body = strstr(line_end + 2, "\r\n\r\n");
    char *payload = "";
    if (body) {
        payload = body + 4;        // указатель на начало тела
    }

    // Теперь у вас есть: uri, qs, payload
    // Собираем ровно ту строку для нормализации:
    char check_buf[BUF_SIZE * 2];
    snprintf(check_buf, sizeof check_buf, "%s?%s\n%s", uri, qs, payload);

    char *norm = normalize_input(check_buf);

    // Для гарантированного вывода добавляем '\n' и fflush:
    fprintf(stderr, "Normalized: %s\n", norm);
    fflush(stderr);
    
    if (detect_sqli(norm)) {
        free(norm);

        const char *resp =
            "HTTP/1.1 403 Forbidden\r\n"
            "Content-Type: text/html\r\n\r\n"
            "<h1>403 Forbidden</h1><p>Blocked by SQLi filter.</p>";
        send(client_fd, resp, strlen(resp), 0);
        close(client_fd);
        return;
    }
    free(norm);
    
    int backend = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in be_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(BACKEND_PORT)
    };
    inet_pton(AF_INET, BACKEND_ADDR, &be_addr.sin_addr);
    if (connect(backend, (struct sockaddr*)&be_addr, sizeof be_addr) < 0) {
        perror("connect backend");
        close(client_fd);
        return;
    }
    // На бек
    send(backend, buf, n, 0);

    // Ответ бека
    while ((n = recv(backend, buf, BUF_SIZE, 0)) > 0) {
        send(client_fd, buf, n, 0);
    }

    close(backend);
    close(client_fd);
}

int main() {
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in proxy_addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(PROXY_PORT)
    };
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
    bind(listenfd, (struct sockaddr*)&proxy_addr, sizeof proxy_addr);
    listen(listenfd, 128);
    printf("Proxy listening on port %d\n", PROXY_PORT);

    while (1) {
        int client = accept(listenfd, NULL, NULL);
        if (client < 0) continue;
        if (fork() == 0) {
            close(listenfd);
            handle_client(client);
            _exit(0);
        } else {
            close(client);
        }
    }
    return 0;
}