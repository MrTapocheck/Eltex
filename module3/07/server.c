#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <signal.h>

#define PORT 9000
#define MAX_CLIENTS 10
#define BUFFER_SIZE 4096

volatile sig_atomic_t running = 1;

void handle_signal(int sig) {
    running = 0;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Создаём сокет
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // Массив клиентских сокетов
    int client_sockets[MAX_CLIENTS] = {0};
    char client_names[MAX_CLIENTS][64];
    memset(client_names, 0, sizeof(client_names));

    struct pollfd fds[MAX_CLIENTS + 1];
    int nfds;

    printf("Сервер запущен на порту %d\n", PORT);

    while (running) {
        // Настраиваем poll
        fds[0].fd = server_fd;
        fds[0].events = POLLIN;
        nfds = 1;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] > 0) {
                fds[nfds].fd = client_sockets[i];
                fds[nfds].events = POLLIN;
                nfds++;
            }
        }

        int ret = poll(fds, nfds, 1000); // таймаут 1 сек
        if (ret < 0) {
            if (errno == EINTR) continue;
            perror("poll");
            break;
        }

        // Новое подключение
        if (fds[0].revents & POLLIN) {
            new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
            if (new_socket < 0) {
                perror("accept");
                continue;
            }

            // Находим свободное место
            int slot = -1;
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    slot = i;
                    break;
                }
            }

            if (slot == -1) {
                // Нет места, отклоняем
                close(new_socket);
                printf("Нет свободных слотов, клиент отклонён\n");
            } else {
                client_sockets[slot] = new_socket;
                printf("Клиент подключился: слот %d, сокет %d\n", slot, new_socket);
            }
        }

        // Обрабатываем клиентов
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sock = client_sockets[i];
            if (sock <= 0) continue;

            // Находим индекс в fds
            int idx = -1;
            for (int j = 1; j < nfds; j++) {
                if (fds[j].fd == sock) {
                    idx = j;
                    break;
                }
            }
            if (idx == -1 || !(fds[idx].revents & POLLIN)) continue;

            char buffer[BUFFER_SIZE];
            ssize_t bytes_read = recv(sock, buffer, sizeof(buffer) - 1, 0);

            if (bytes_read <= 0) {
                // Клиент отключился
                close(sock);
                client_sockets[i] = 0;
                printf("Клиент из слота %d отключился\n", i);
                continue;
            }

            buffer[bytes_read] = '\0';

            // Проверяем команды
            if (strncmp(buffer, "/join ", 6) == 0) {
                strncpy(client_names[i], buffer + 6, 63);
                client_names[i][63] = '\0';
                printf("Клиент %s подключился\n", client_names[i]);

                // Рассылаем остальным сообщение о подключении
                char msg[BUFFER_SIZE];
                snprintf(msg, sizeof(msg), "%s: подключился к чату\n", client_names[i]);
                for (int j = 0; j < MAX_CLIENTS; j++) {
                    if (client_sockets[j] > 0 && j != i) {
                        send(client_sockets[j], msg, strlen(msg), 0);
                    }
                }
            }
            else if (strncmp(buffer, "/leave ", 7) == 0) {
                // Сообщение об отключении уже будет обработано при close, но можно отправить сразу
                char msg[BUFFER_SIZE];
                snprintf(msg, sizeof(msg), "%s: покинул чат\n", client_names[i]);
                for (int j = 0; j < MAX_CLIENTS; j++) {
                    if (client_sockets[j] > 0 && j != i) {
                        send(client_sockets[j], msg, strlen(msg), 0);
                    }
                }
                close(sock);
                client_sockets[i] = 0;
                printf("Клиент %s отключился\n", client_names[i]);
                client_names[i][0] = '\0';
            }
            else if (strncmp(buffer, "/file ", 6) == 0) {
                // Получаем имя файла и размер
                char filename[256];
                long long filesize;
                if (sscanf(buffer + 6, "%255s %lld", filename, &filesize) == 2) {
                    printf("Получен файл %s от %s, размер %lld байт\n", filename, client_names[i], filesize);
                    // Рассылаем команду остальным
                    char header[512];
                    snprintf(header, sizeof(header), "/file %s %lld\n", filename, filesize);
                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (client_sockets[j] > 0 && j != i) {
                            send(client_sockets[j], header, strlen(header), 0);
                        }
                    }

                    // Пересылаем содержимое файла
                    long long remaining = filesize;
                    while (remaining > 0) {
                        ssize_t n = recv(sock, buffer, sizeof(buffer), 0);
                        if (n <= 0) break;
                        // Рассылаем всем
                        for (int j = 0; j < MAX_CLIENTS; j++) {
                            if (client_sockets[j] > 0 && j != i) {
                                send(client_sockets[j], buffer, n, 0);
                            }
                        }
                        remaining -= n;
                    }
                }
            }
            else {
                // Обычное сообщение
                char msg[BUFFER_SIZE];
                snprintf(msg, sizeof(msg), "%s: %s", client_names[i], buffer);
                for (int j = 0; j < MAX_CLIENTS; j++) {
                    if (client_sockets[j] > 0 && j != i) {
                        send(client_sockets[j], msg, strlen(msg), 0);
                    }
                }
            }
        }
    }

    // Завершение работы: закрываем все сокеты
    close(server_fd);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] > 0) close(client_sockets[i]);
    }
    printf("Сервер остановлен.\n");
    return 0;
}