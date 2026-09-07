#ifndef COMMON_POSIX_H
#define COMMON_POSIX_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>
#include <signal.h>

#define SHM_NAME "/my_shm_posix"   // имя разделяемой памяти POSIX
#define SEM_NAME "/my_sem_posix"   // имя семафора POSIX
#define SEGMENT_SIZE 4096          // размер сегмента памяти

struct header {
    size_t total_size;
    size_t free_offset;
    int producer_done;
    int all_done;
};

struct block {
    int id;         // порядковый номер блока
    int count;      // >0 - необработан, 0 - обработан
    int next;       // смещение следующего блока (0 = нет)
    int data[];     // гибкий массив
};

// Функции для работы с семафором POSIX
sem_t* create_semaphore();
sem_t* get_semaphore();
void sem_lock(sem_t* sem);
void sem_unlock(sem_t* sem);
void remove_semaphore(sem_t* sem);

// Функции для работы с разделяемой памятью POSIX
int create_shm();
int get_shm();
void* attach_shm(int fd);
void detach_shm(void* addr);
void remove_shm(int fd);

#endif