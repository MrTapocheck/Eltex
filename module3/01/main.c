#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLOCK_SIZE          1024 
#define MAX_FILENAME        256   
#define PIPE_NAME_MAX       256

enum exit_code {
    EXIT_OK          = 0,
    EXIT_ERR_USAGE   = 1,
    EXIT_ERR_PIPE    = 2,
    EXIT_ERR_FORK    = 3,
    EXIT_ERR_MEMORY  = 4,
    EXIT_ERR_FIFO    = 5
};

enum pipe_type {
    PIPE_UNNAMED = 0,  // Неименованный канал (pipe)
    PIPE_NAMED = 1     // Именованный канал (FIFO)
};

typedef struct {
    enum pipe_type pipe_type;
    char pipe_name[256];
    char file_count;
    char **files;
} ProgramParams;

int main(int argc, char *argv[])
{
    ProgramParams params;
    
    return EXIT_OK;
}