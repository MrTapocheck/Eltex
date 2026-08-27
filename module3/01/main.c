#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>

#define BUF_SIZE 4096
#define NAME_SIZE 256

typedef struct {
    char name[NAME_SIZE];
    long size;
} FileInfo;

int read_all(int fd, void *buf, size_t n)
{
    size_t done = 0;

    while (done < n) {
        ssize_t r = read(fd, (char *)buf + done, n - done);

        if (r == 0)
            return 0;
        if (r < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }

        done += r;
    }

    return 1;
}

int write_all(int fd, const void *buf, size_t n)
{
    size_t done = 0;

    while (done < n) {
        ssize_t w = write(fd, (const char *)buf + done, n - done);

        if (w < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }

        done += w;
    }

    return 0;
}

int main(int argc, char **argv)
{
    int named = 0, files_count = 0;
    char pipe_name[NAME_SIZE] = "";
    char **files = malloc(argc * sizeof(char *));

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-p")) {
            if (++i == argc) {
                fprintf(stderr, "После -p нужно имя канала\n");
                return 1;
            }
            named = 1;
            strcpy(pipe_name, argv[i]);
        } else {
            files[files_count++] = argv[i];
        }
    }

    if (!files_count) {
        fprintf(stderr, "Укажите хотя бы один файл\n");
        return 1;
    }

    int ready[2], data[2] = {-1, -1};

    if (pipe(ready) == -1) {
        perror("pipe");
        return 1;
    }

    if (named) {
        if (mkfifo(pipe_name, 0666) == -1 && errno != EEXIST) {
            perror("mkfifo");
            return 1;
        }
    } else if (pipe(data) == -1) {
        perror("pipe");
        return 1;
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return 1;
    }

    //ребёнок
    if (pid == 0) {
        int in;

        close(ready[0]);

        if (named) {
            in = open(pipe_name, O_RDONLY);
            if (in == -1) {
                perror("open fifo");
                exit(1);
            }
        } else {
            close(data[1]);
            in = data[0];
        }

        char ready_byte = 1;
        write_all(ready[1], &ready_byte, 1);
        close(ready[1]);

        while (1) {
            FileInfo info;
            char buf[BUF_SIZE];

            if (read_all(in, &info, sizeof(info)) != 1)
                break;

            if (info.size < 0)
                break;

            char copy[NAME_SIZE + 6];
            snprintf(copy, sizeof(copy), "%s.copy", info.name);

            int out = open(copy, O_WRONLY | O_CREAT | O_TRUNC, 0644);

            long left = info.size;

            while (left > 0) {
                size_t n = left < BUF_SIZE ? left : BUF_SIZE;

                ssize_t r = read(in, buf, n);

                if (r == 0)
                    break;

                if (r < 0) {
                    if (errno == EINTR)
                        continue;
                    break;
                }

                if (out != -1)
                    write_all(out, buf, r);

                left -= r;
            }

            if (out != -1)
                close(out);
        }

        close(in);
        exit(0);
    }

    close(ready[1]);

    int out;

    if (named) {
        out = open(pipe_name, O_WRONLY);
        if (out == -1) {
            perror("open fifo");
            return 1;
        }
    } else {
        close(data[0]);
        out = data[1];
    }

    char ready_byte;

    if (read_all(ready[0], &ready_byte, 1) != 1) {
        fprintf(stderr, "Ребёнок не готов\n");
        return 1;
    }

    close(ready[0]);

    for (int i = 0; i < files_count; i++) {
        FileInfo info = {0};

        strncpy(info.name, files[i], NAME_SIZE - 1);

        int fd = open(files[i], O_RDONLY);

        if (fd == -1) {
            fprintf(stderr, "Файл '%s' не существует\n", files[i]);
            info.size = 0;
            write_all(out, &info, sizeof(info));
            continue;
        }

        info.size = lseek(fd, 0, SEEK_END);
        lseek(fd, 0, SEEK_SET);

        write_all(out, &info, sizeof(info));

        char buf[BUF_SIZE];
        ssize_t n;

        while ((n = read(fd, buf, sizeof(buf))) > 0)
            write_all(out, buf, n);

        close(fd);
    }

    FileInfo end = {0};
    end.size = -1;
    write_all(out, &end, sizeof(end));

    close(out);

    waitpid(pid, NULL, 0);

    if (named)
        unlink(pipe_name);

    free(files);

    return 0;
}