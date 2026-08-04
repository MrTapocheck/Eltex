#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>

#define BLOCK_SIZE          1024 
#define MAX_FILENAME        256   
#define PIPE_NAME_MAX       256
#define READY_MSG_LEN       5

enum exit_code {
    EXIT_OK          = 0,
    EXIT_ERR_USAGE   = 1,
    EXIT_ERR_PIPE    = 2,
    EXIT_ERR_FORK    = 3,
    EXIT_ERR_MEMORY  = 4,
    EXIT_ERR_FIFO    = 5,
    EXIT_ERR_OPEN    = 6,
    EXIT_ERR_IO      = 7
};

enum pipe_type {
    PIPE_UNNAMED = 0,  // Неименованный канал (pipe)
    PIPE_NAMED = 1     // Именованный канал (FIFO)
};

typedef struct {
    enum pipe_type pipe_type;
    char pipe_name[PIPE_NAME_MAX];
    int file_count;
    char **files;
} ProgramParams;

static int parse_args(int argc, char *argv[], ProgramParams *params);
static int create_pipes(ProgramParams *params, int data_pipe[2], int ready_pipe[2]);
static ssize_t write_all(int fd, const void *buf, size_t count);
static ssize_t read_all(int fd, void *buf, size_t count);
static void parent_process(ProgramParams *params, int data_pipe[2], int ready_pipe[2]);
static void child_process(ProgramParams *params, int data_pipe[2], int ready_pipe[2]);
static void cleanup(ProgramParams *params, int data_pipe[2], int ready_pipe[2]);

int main(int argc, char *argv[])
{
	ProgramParams params = {0};
	int data_pipe[2] = {-1, -1};
	int ready_pipe[2] = {-1, -1};
	pid_t pid;
	int ret;

	ret = parse_args(argc, argv, &params);
	if (ret != EXIT_OK)
		return ret;

	ret = create_pipes(&params, data_pipe, ready_pipe);
	if (ret != EXIT_OK)
		return ret;

	pid = fork();
	if (pid < 0) {
		perror("fork");
		cleanup(&params, data_pipe, ready_pipe);
		return EXIT_ERR_FORK;
	}

	if (pid == 0) {
		child_process(&params, data_pipe, ready_pipe);
		_exit(EXIT_OK);
	}

	parent_process(&params, data_pipe, ready_pipe);

	wait(NULL);

	cleanup(&params, data_pipe, ready_pipe);
	return EXIT_OK;
}

static int parse_args(int argc, char *argv[], ProgramParams *params)
{
	int i, file_idx;

	params->pipe_type = PIPE_UNNAMED;
	params->pipe_name[0] = '\0';
	params->file_count = 0;
	params->files = NULL;

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-p") == 0) {
			if (i + 1 >= argc) {
				fprintf(stderr, "Ошибка: после -p нужно указать имя канала\n");
				return EXIT_ERR_USAGE;
			}
			params->pipe_type = PIPE_NAMED;
			strncpy(params->pipe_name, argv[i + 1], PIPE_NAME_MAX - 1);
			params->pipe_name[PIPE_NAME_MAX - 1] = '\0';
			i++;
		} else {
			params->file_count++;
		}
	}

	if (params->file_count == 0) {
		fprintf(stderr, "Использование: %s [-p pipe_name] файл1 [файл2 ...]\n",
			argv[0]);
		return EXIT_ERR_USAGE;
	}

	params->files = malloc((size_t)params->file_count * sizeof(char *));
	if (!params->files) {
		perror("malloc");
		return EXIT_ERR_MEMORY;
	}

	file_idx = 0;
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-p") == 0) {
			i++;
			continue;
		}
		params->files[file_idx++] = argv[i];
	}

	return EXIT_OK;
}

static int create_pipes(ProgramParams *params, int data_pipe[2], int ready_pipe[2])
{
	if (pipe(ready_pipe) == -1) {
		perror("pipe ready");
		return EXIT_ERR_PIPE;
	}

	if (params->pipe_type == PIPE_UNNAMED) {
		if (pipe(data_pipe) == -1) {
			perror("pipe data");
			close(ready_pipe[0]);
			close(ready_pipe[1]);
			return EXIT_ERR_PIPE;
		}
	} else {
		if (mkfifo(params->pipe_name, 0666) == -1 && errno != EEXIST) {
			perror("mkfifo");
			close(ready_pipe[0]);
			close(ready_pipe[1]);
			return EXIT_ERR_FIFO;
		}

		data_pipe[0] = -1;
		data_pipe[1] = -1;
	}

	return EXIT_OK;
}

static ssize_t write_all(int fd, const void *buf, size_t count)
{
	const char *p = buf;
	size_t left = count;

	while (left > 0) {
		ssize_t n = write(fd, p, left);
		if (n < 0) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		if (n == 0)
			return -1;

		p += (size_t)n;
		left -= (size_t)n;
	}
	return (ssize_t)count;
}

static ssize_t read_all(int fd, void *buf, size_t count)
{
	char *p = buf;
	size_t left = count;

	while (left > 0) {
		ssize_t n = read(fd, p, left);
		if (n < 0) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		if (n == 0)
			return (ssize_t)(count - left);

		p += (size_t)n;
		left -= (size_t)n;
	}
	return (ssize_t)count;
}

static void cleanup(ProgramParams *params, int data_pipe[2], int ready_pipe[2])
{
	if (data_pipe[0] != -1)
		close(data_pipe[0]);
	if (data_pipe[1] != -1)
		close(data_pipe[1]);

	if (ready_pipe[0] != -1)
		close(ready_pipe[0]);
	if (ready_pipe[1] != -1)
		close(ready_pipe[1]);

	if (params->pipe_type == PIPE_NAMED && params->pipe_name[0] != '\0')
		unlink(params->pipe_name);

	free(params->files);
	params->files = NULL;
}

static void child_process(ProgramParams *params, int data_pipe[2], int ready_pipe[2])
{
	int data_rd = -1;
	int ready_wr = ready_pipe[1];

	close(ready_pipe[0]);

	if (params->pipe_type == PIPE_UNNAMED) {
		close(data_pipe[1]);
		data_rd = data_pipe[0];
	}

	for (;;) {
		char header[MAX_FILENAME + 64];
		char copy_name[sizeof(header) + 16];
		char buf[BLOCK_SIZE];
		char *colon;
		long size, left;
		int out_fd;
		ssize_t n;
		size_t i;

		if (write_all(ready_wr, "READY", READY_MSG_LEN) != READY_MSG_LEN)
			break;

		if (params->pipe_type == PIPE_NAMED) {
			data_rd = open(params->pipe_name, O_RDONLY);
			if (data_rd < 0) {
				perror("child open fifo");
				break;
			}
		}

		i = 0;
		while (i < sizeof(header) - 1) {
			n = read(data_rd, &header[i], 1);
			if (n <= 0)
				goto done;
			if (header[i] == '\0')
				break;
			i++;
		}
		header[i] = '\0';

		if (strcmp(header, "END") == 0)
			break;

		colon = strchr(header, ':');
		if (!colon)
			goto close_data;

		*colon = '\0';
		size = atol(colon + 1);
		if (size < 0)
			size = 0;

		snprintf(copy_name, sizeof(copy_name), "%s.copy", header);

		out_fd = open(copy_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (out_fd < 0) {
			perror("child create copy");
			left = size;
			while (left > 0) {
				n = read(data_rd, buf, (size_t)(left > BLOCK_SIZE ? BLOCK_SIZE : left));
				if (n <= 0)
					break;
				left -= n;
			}
			goto close_data;
		}

		left = size;
		while (left > 0) {
			n = read(data_rd, buf, (size_t)(left > BLOCK_SIZE ? BLOCK_SIZE : left));
			if (n <= 0)
				break;
			if (write_all(out_fd, buf, (size_t)n) != n)
				break;
			left -= n;
		}

		close(out_fd);

close_data:
		if (params->pipe_type == PIPE_NAMED && data_rd >= 0) {
			close(data_rd);
			data_rd = -1;
		}
	}

done:
	if (data_rd >= 0)
		close(data_rd);
	close(ready_wr);
}

static void parent_process(ProgramParams *params, int data_pipe[2], int ready_pipe[2])
{
	int data_wr = -1;
	int ready_rd = ready_pipe[0];
	int i;

	close(ready_pipe[1]);

	if (params->pipe_type == PIPE_UNNAMED) {
		close(data_pipe[0]);
		data_wr = data_pipe[1];
	}

	for (i = 0; i < params->file_count; i++) {
		char ready[READY_MSG_LEN + 1];
		char header[MAX_FILENAME + 64];
		char buf[BLOCK_SIZE];
		int fd;
		off_t size;
		ssize_t n;

		if (read_all(ready_rd, ready, READY_MSG_LEN) != READY_MSG_LEN)
			break;
		ready[READY_MSG_LEN] = '\0';
		if (strcmp(ready, "READY") != 0)
			break;

		if (params->pipe_type == PIPE_NAMED) {

            data_wr = open(params->pipe_name, O_WRONLY);
			if (data_wr < 0) {
				perror("parent open fifo");
				break;
			}
		}

		fd = open(params->files[i], O_RDONLY);
		if (fd < 0) {
			fprintf(stderr, "Файл '%s' не существует\n", params->files[i]);
			snprintf(header, sizeof(header), "%s:0", params->files[i]);
			write_all(data_wr, header, strlen(header) + 1);
			goto next;
		}

		size = lseek(fd, 0, SEEK_END);
		lseek(fd, 0, SEEK_SET);
		if (size < 0)
			size = 0;

		snprintf(header, sizeof(header), "%s:%ld", params->files[i], (long)size);
		if (write_all(data_wr, header, strlen(header) + 1) < 0) {
			close(fd);
			break;
		}

		while ((n = read(fd, buf, BLOCK_SIZE)) > 0) {
			if (write_all(data_wr, buf, (size_t)n) != n)
				break;
		}

		close(fd);

next:
		if (params->pipe_type == PIPE_NAMED && data_wr >= 0) {
			close(data_wr);
			data_wr = -1;
		}
	}

	{
		char ready[READY_MSG_LEN + 1];
		if (read_all(ready_rd, ready, READY_MSG_LEN) == READY_MSG_LEN) {
			if (params->pipe_type == PIPE_NAMED)
				data_wr = open(params->pipe_name, O_WRONLY);

			if (data_wr >= 0)
				write_all(data_wr, "END", 4);
		}
	}

	if (data_wr >= 0)
		close(data_wr);
	close(ready_rd);
}