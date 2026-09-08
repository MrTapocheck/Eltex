#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#define BUFFER_SIZE 4096

volatile sig_atomic_t running = 1;
int sockfd;
char username[64];

void handle_signal(int sig) {
    running = 0;
}

// Функция отправки файла
void send_file(const char *path) {
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        perror("fopen");
        return;
    }

    fseek(fp, 0, SEEK_END);
    long long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Извлекаем имя файла из пути
    const char *filename = strrchr(path, '/');
    filename = filename ? filename + 1 : path;

    char header[512];
    snprintf(header, sizeof(header), "/file %s %lld\n", filename, size);
    send(sockfd, header, strlen(header), 0);

    char buffer[BUFFER_SIZE];
    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        send(sockfd, buffer, n, 0);
    }
    fclose(fp);
    printf("Файл %s отправлен\n", filename);
}

// Функция приёма файла
void receive_file(const char *filename, long long size) {
    // Создаём директорию received, если нет
    mkdir("received", 0777);
    char path[512];
    snprintf(path, sizeof(path), "received/%s", filename);

    FILE *fp = fopen(path, "wb");
    if (!fp) {
        perror("fopen");
        return;
    }

    long long remaining = size;
    char buffer[BUFFER_SIZE];
    while (remaining > 0) {
        ssize_t n = recv(sockfd, buffer, sizeof(buffer), 0);
        if (n <= 0) {
            perror("recv file");
            break;
        }
        fwrite(buffer, 1, n, fp);
        remaining -= n;
    }
    fclose(fp);
    printf("Получен файл: %s (%lld байт)\n", path, size);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Использование: %s <IP сервера> <имя>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *server_ip = argv[1];
    strncpy(username, argv[2], sizeof(username) - 1);
    username[63] = '\0';

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9000);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Неверный IP адрес\n");
        exit(EXIT_FAILURE);
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // Отправляем имя серверу
    char join_msg[128];
    snprintf(join_msg, sizeof(join_msg), "/join %s\n", username);
    send(sockfd, join_msg, strlen(join_msg), 0);

    printf("Добро пожаловать в чат, %s!\n", username);
    printf("Введите сообщение или /send <путь_к_файлу> для отправки файла.\n");

    fd_set readfds;
    char recv_buffer[BUFFER_SIZE];

    while (running) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(sockfd, &readfds);
        int maxfd = (sockfd > STDIN_FILENO) ? sockfd : STDIN_FILENO;

        int ret = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (ret < 0) {
            if (errno == EINTR) continue;
            perror("select");
            break;
        }

        // Данные с клавиатуры
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            char input[BUFFER_SIZE];
            if (fgets(input, sizeof(input), stdin) == NULL) {
                break;
            }
            // Убираем перевод строки
            size_t len = strlen(input);
            if (len > 0 && input[len-1] == '\n') input[len-1] = '\0';

            if (strlen(input) == 0) continue;

            if (strncmp(input, "/send ", 6) == 0) {
                send_file(input + 6);
            } else {
                send(sockfd, input, strlen(input), 0);
            }
        }

        // Данные от сервера
        if (FD_ISSET(sockfd, &readfds)) {
            ssize_t n = recv(sockfd, recv_buffer, sizeof(recv_buffer) - 1, 0);
            if (n <= 0) {
                printf("Соединение закрыто сервером.\n");
                break;
            }
            recv_buffer[n] = '\0';

            // Проверяем, не начало ли это файла
            if (strncmp(recv_buffer, "/file ", 6) == 0) {
                char filename[256];
                long long size;
                if (sscanf(recv_buffer + 6, "%255s %lld", filename, &size) == 2) {
                    receive_file(filename, size);
                }
            } else {
                printf("%s", recv_buffer);
                fflush(stdout);
            }
        }
    }

    // Отправляем сообщение об отключении
    char leave_msg[128];
    snprintf(leave_msg, sizeof(leave_msg), "/leave %s\n", username);
    send(sockfd, leave_msg, strlen(leave_msg), 0);

    close(sockfd);
    printf("Вы покинули чат.\n");
    return 0;
}