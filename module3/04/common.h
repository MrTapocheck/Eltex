#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <time.h>
#include <errno.h>

#define SHM_KEY 1234          // ключ разделяемой памяти
#define SEM_KEY 5678          // ключ семафора
#define SEGMENT_SIZE 4096     // размер сегмента памяти

struct header {
    size_t total_size;
    size_t free_offset;
    int producer_done;
    int all_done;
};

struct block {
    int id;
    int count;      // >0 - необработан, 0 - обработан
    int next;       // смещение следующего блока (0 = нет)
    int data[];     // гибкий массив
};

// Функции для работы с семафором
int create_semaphore();
int get_semaphore();
void sem_lock(int semid);
void sem_unlock(int semid);
void remove_semaphore(int semid);

// Функции для работы с разделяемой памятью
int create_shm();
int get_shm();
void* attach_shm(int shmid);
void detach_shm(void* addr);
void remove_shm(int shmid);

#endif