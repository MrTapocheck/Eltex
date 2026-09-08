#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>

#define PORT 8888
#define BROADCAST_ADDR "255.255.255.255"
#define MAX_MSG 1024

int sockfd;
char username[64];
struct sockaddr_in bcast_addr;
volatile sig_atomic_t running = 1;

void handle_signal(int sig) {
    running = 0;
}

void *receiver_thread(void *arg) {
    char buffer[MAX_MSG];
    struct sockaddr_in src_addr;
    socklen_t addr_len = sizeof(src_addr);

    while (running) {
        int n = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                         (struct sockaddr *)&src_addr, &addr_len);
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("recvfrom");
            break;
        }
        buffer[n] = '\0';
        
        // пропускаем сообщения, начинающиеся с нашего имени.
        if (strncmp(buffer, username, strlen(username)) == 0 &&
            buffer[strlen(username)] == ':') {
            continue; // своё сообщение
        }

        printf("%s\n", buffer);
        fflush(stdout);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        strncpy(username, argv[1], sizeof(username) - 1);
    } else {
        // Имя по умолчанию из переменной окружения USER или "user"
        char *env_user = getenv("USER");
        if (env_user) {
            snprintf(username, sizeof(username), "%s", env_user);
        } else {
            snprintf(username, sizeof(username), "user%d", getpid());
        }
    }

    // создаём UDP сокет
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    // разрешаем broadcast
    int broadcast = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
        perror("setsockopt SO_BROADCAST");
        exit(1);
    }

    // заполняем адрес для приёма
    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(PORT);
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        perror("bind");
        exit(1);
    }

    // адрес для broadcast отправки
    memset(&bcast_addr, 0, sizeof(bcast_addr));
    bcast_addr.sin_family = AF_INET;
    bcast_addr.sin_port = htons(PORT);
    if (inet_aton(BROADCAST_ADDR, &bcast_addr.sin_addr) == 0) {
        fprintf(stderr, "Invalid broadcast address\n");
        exit(1);
    }

    // Обработка SIGINT для корректного завершения
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // запускаем поток приёма сообщений
    pthread_t tid;
    if (pthread_create(&tid, NULL, receiver_thread, NULL) != 0) {
        perror("pthread_create");
        exit(1);
    }

    //сообщение о подключении
    char join_msg[MAX_MSG];
    snprintf(join_msg, sizeof(join_msg), "%s: присоединился к чату", username);
    sendto(sockfd, join_msg, strlen(join_msg), 0,
           (struct sockaddr *)&bcast_addr, sizeof(bcast_addr));

    printf("Добро пожаловать в чат, %s!\n", username);
    printf("Введите сообщение и нажмите Enter (Ctrl+C для выхода):\n");

    //отправка
    char msg[MAX_MSG];
    while (running) {
        if (fgets(msg, sizeof(msg), stdin) == NULL) {
            break; // EOF
        }
        // убираем перевод строки
        size_t len = strlen(msg);
        if (len > 0 && msg[len-1] == '\n') {
            msg[len-1] = '\0';
        }

        if (strlen(msg) == 0) continue;

        // формируем сообщение
        char full_msg[MAX_MSG + sizeof(username)+1];
        snprintf(full_msg, sizeof(full_msg), "%s: %s", username, msg);
        sendto(sockfd, full_msg, strlen(full_msg), 0,
               (struct sockaddr *)&bcast_addr, sizeof(bcast_addr));
    }

    // сообщение об отключении
    char leave_msg[MAX_MSG];
    snprintf(leave_msg, sizeof(leave_msg), "%s: покинул чат", username);
    sendto(sockfd, leave_msg, strlen(leave_msg), 0,
           (struct sockaddr *)&bcast_addr, sizeof(bcast_addr));

    // ожидаем завершения потока приёма
    pthread_cancel(tid);
    pthread_join(tid, NULL);

    close(sockfd);
    printf("Вы покинули чат.\n");
    return 0;
}