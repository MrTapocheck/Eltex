#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>

#define TEXT 512
#define END 99

volatile sig_atomic_t running = 1;
mqd_t in_queue, out_queue;

void stop(int sig)
{
    (void)sig;
    running = 0;
    
    char end_msg[] = "end";
    mq_send(out_queue, end_msg, strlen(end_msg) + 1, END); //чтобы уведомить собеседника
    mq_send(in_queue, end_msg, strlen(end_msg) + 1, END); //чтобы разблокировать свой дочерний процесс
}

void receive_messages(mqd_t queue)
{
    char text[TEXT];
    unsigned int priority;
    
    while (running) {
        ssize_t received = mq_receive(queue, text, TEXT, &priority);
        
        if (received == -1) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        
        if (priority == END) {
            printf("\n[Чат завершен]\n");
            running = 0;
            break;
        }
        
        printf("\n[Получено]: %s\n", text);
        printf("> ");
        fflush(stdout);
    }
}

void send_messages(mqd_t queue)
{
    char text[TEXT];
    
    while (running) {
        printf("> ");
        fflush(stdout);
        
        if (fgets(text, TEXT, stdin) == NULL) {
            break;
        }
        
        if (!running) break;
        
        // удалить символ новой строки
        size_t len = strlen(text);
        if (len > 0 && text[len-1] == '\n') {
            text[len-1] = '\0';
            len--;
        }
        
        if (len > 0 && mq_send(queue, text, len + 1, 1) == -1) {
            perror("mq_send");
            break;
        }
    }
}

int main(int argc, char **argv)
{
    char queue1[100];
    char queue2[100];
    
    struct mq_attr attr;
    struct sigaction action;
    
    int owner = 0;
    pid_t pid;
    int status;
    
    if (argc != 2) {
        fprintf(stderr, "Использование: %s <имя>\n", argv[0]);
        return 1;
    }
    
    snprintf(queue1, sizeof(queue1), "/%s_1", argv[1]);
    snprintf(queue2, sizeof(queue2), "/%s_2", argv[1]);
    
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = TEXT;
    attr.mq_curmsgs = 0;
    
    // обработчик сигнала до fork
    action.sa_handler = stop;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGINT, &action, NULL);
    
    in_queue = mq_open(queue1, O_CREAT | O_EXCL | O_RDWR, 0666, &attr);
    
    if (in_queue != (mqd_t)-1) {
        owner = 1;
        out_queue = mq_open(queue2, O_CREAT | O_RDWR, 0666, &attr);
        if (out_queue == (mqd_t)-1) {
            perror("mq_open queue2");
            mq_close(in_queue);
            mq_unlink(queue1);
            return 1;
        }
        printf("[Созданы очереди. Ожидание собеседника...]\n");
    } else {
        in_queue = mq_open(queue2, O_RDWR);
        out_queue = mq_open(queue1, O_RDWR);
        
        if (in_queue == (mqd_t)-1 || out_queue == (mqd_t)-1) {
            fprintf(stderr, "Не удалось открыть очереди\n");
            return 1;
        }
        printf("[Подключение к существующим очередям]\n");
    }
    
    printf("[Чат запущен]\n");
    
    pid = fork();
    
    if (pid == -1) {
        perror("fork");
        return 1;
    }
    
    if (pid == 0) {
        // дочерний процесс - только прием сообщений
        signal(SIGINT, SIG_IGN);  // игнор SIGINT в дочернем
        receive_messages(in_queue);
        
        mq_close(in_queue);
        mq_close(out_queue);
        
        _exit(0);
    }
    
    // родительский процесс - только отправка сообщений
    send_messages(out_queue);
    
    // ждем завершения дочернего процесса
    waitpid(pid, &status, 0);
    
    mq_close(in_queue);
    mq_close(out_queue);
    
    if (owner) {
        mq_unlink(queue1);
        mq_unlink(queue2);
        printf("[Очереди удалены]\n");
    }
    
    return 0;
}