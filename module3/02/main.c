#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/msg.h>

#define KEY 1234
#define MAX 100
#define TEXT 512

typedef struct {
    long type;
    char text[TEXT];
} Message;

typedef struct {
    pid_t pid;
    char topic[64];
} Subscriber;

Subscriber subscribers[MAX];
pid_t publishers[MAX];

int subscribers_count = 0;
int publishers_count = 0;
int queue_id;
int wait = 1;

void stop(int sig)
{
    (void)sig;
    wait = 0;
}

void subscribe(pid_t pid, char *topic)
{
    if (subscribers_count >= MAX)
        return;

    subscribers[subscribers_count].pid = pid;
    strcpy(subscribers[subscribers_count].topic, topic);
    subscribers_count++;
}

void unsubscribe(pid_t pid, char *topic)
{
    for (int i = 0; i < subscribers_count; i++) {
        if (subscribers[i].pid == pid &&
            strcmp(subscribers[i].topic, topic) == 0) {

            subscribers[i] = subscribers[subscribers_count - 1];
            subscribers_count--;
            return;
        }
    }
}

void broker()
{
    Message msg;

    signal(SIGINT, stop);

    queue_id = msgget(KEY, IPC_CREAT | IPC_EXCL | 0666);

    if (queue_id == -1) {
        printf("Брокер уже запущен\n");
        return;
    }

    printf("Брокер запущен, pid=%d\n", getpid());

    while (wait) {
        if (msgrcv(queue_id, &msg, TEXT, 1, 0) == -1)
            break;

        char command[20];
        char topic[64];
        int pid;

        if (sscanf(msg.text, "%19[^,],%d,%63[^,]",
                   command, &pid, topic) != 3)
            continue;

        if (strcmp(command, "subscribe") == 0) {
            subscribe(pid, topic);
            printf("Подписчик %d -> %s\n", pid, topic);
        }

        else if (strcmp(command, "unsubscribe") == 0) {
            unsubscribe(pid, topic);
        }

        else if (strcmp(command, "send") == 0) {
            if (publishers_count < MAX)
                publishers[publishers_count++] = pid;

            for (int i = 0; i < subscribers_count; i++) {
                if (strcmp(subscribers[i].topic, topic) == 0) {
                    msg.type = subscribers[i].pid;

                    msgsnd(queue_id, &msg,
                           strlen(msg.text) + 1, 0);
                }
            }
        }
    }

    for (int i = 0; i < publishers_count; i++)
        kill(publishers[i], SIGINT);

    for (int i = 0; i < subscribers_count; i++)
        kill(subscribers[i].pid, SIGINT);

    msgctl(queue_id, IPC_RMID, NULL);

    printf("Брокер остановлен\n");
}

void publisher(char *topic, char *text)
{
    Message msg;

    queue_id = msgget(KEY, 0666);

    if (queue_id == -1) {
        printf("Очередь недоступна\n");
        return;
    }

    msg.type = 1;

    sprintf(msg.text, "send,%d,%s,%s",
            getpid(), topic, text);

    msgsnd(queue_id, &msg,
           strlen(msg.text) + 1, 0);

    printf("Сообщение отправлено\n");
}

void subscriber(int count, char **topics)
{
    Message msg;

    queue_id = msgget(KEY, 0666);

    if (queue_id == -1) {
        printf("Очередь недоступна\n");
        return;
    }

    signal(SIGINT, stop);

    for (int i = 0; i < count; i++) {
        msg.type = 1;

        sprintf(msg.text, "subscribe,%d,%s",
                getpid(), topics[i]);

        msgsnd(queue_id, &msg,
               strlen(msg.text) + 1, 0);
    }

    while (wait) {
        if (msgrcv(queue_id, &msg, TEXT, getpid(), 0) == -1)
            break;

        printf("Получено: %s\n", msg.text);
    }

    for (int i = 0; i < count; i++) {
        msg.type = 1;

        sprintf(msg.text, "unsubscribe,%d,%s",
                getpid(), topics[i]);

        msgsnd(queue_id, &msg,
               strlen(msg.text) + 1, 0);
    }
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Использование:\n");
        printf("  %s -b\n", argv[0]);
        printf("  %s -p тема сообщение\n", argv[0]);
        printf("  %s -s тема1 [тема2 ...]\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "-b") == 0) {
        broker();
    }

    else if (strcmp(argv[1], "-p") == 0) {
        if (argc != 4) {
            printf("Использование: %s -p тема сообщение\n", argv[0]);
            return 1;
        }

        publisher(argv[2], argv[3]);
    }
    else if (strcmp(argv[1], "-s") == 0) {
        if (argc < 3) {
            printf("Укажите хотя бы одну тему\n");
            return 1;
        }
        subscriber(argc - 2, &argv[2]);
    }
    else {
        printf("Неизвестный режим\n");
        return 1;
    }

    return 0;
}