#include "common_posix.h"

// ---------------------- Семафоры POSIX -------------------------

sem_t* create_semaphore() {
    // Создаём семафор с начальным значением 1 (мьютекс)
    sem_t* sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0666, 1);
    if (sem == SEM_FAILED) {
        if (errno == EEXIST) {
            fprintf(stderr, "Семафор уже существует. Запустите producer первым.\n");
            exit(1);
        } else {
            perror("sem_open");
            exit(1);
        }
    }
    return sem;
}

sem_t* get_semaphore() {
    sem_t* sem;
    // Ожидаем появления семафора (до 30 секунд)
    for (int i = 0; i < 30; i++) {
        sem = sem_open(SEM_NAME, 0);  // открываем существующий
        if (sem != SEM_FAILED) return sem;
        if (errno != ENOENT) {
            perror("sem_open");
            exit(1);
        }
        sleep(1);
    }
    fprintf(stderr, "Семафор не появился.\n");
    exit(1);
}

void sem_lock(sem_t* sem) {
    if (sem_wait(sem) == -1) {
        perror("sem_wait");
        exit(1);
    }
}

void sem_unlock(sem_t* sem) {
    if (sem_post(sem) == -1) {
        perror("sem_post");
        exit(1);
    }
}

void remove_semaphore(sem_t* sem) {
    if (sem_close(sem) == -1) {
        perror("sem_close");
    }
    if (sem_unlink(SEM_NAME) == -1) {
        perror("sem_unlink");
    }
}

// ------------------- Разделяемая память POSIX -------------------

int create_shm() {
    int fd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0666);
    if (fd == -1) {
        if (errno == EEXIST) {
            fprintf(stderr, "Разделяемая память уже существует. Запустите producer первым.\n");
            exit(1);
        } else {
            perror("shm_open");
            exit(1);
        }
    }
    // Задаём размер сегмента
    if (ftruncate(fd, SEGMENT_SIZE) == -1) {
        perror("ftruncate");
        exit(1);
    }
    return fd;
}

int get_shm() {
    int fd;
    for (int i = 0; i < 30; i++) {
        fd = shm_open(SHM_NAME, O_RDWR, 0666);
        if (fd != -1) return fd;
        if (errno != ENOENT) {
            perror("shm_open");
            exit(1);
        }
        sleep(1);
    }
    fprintf(stderr, "Разделяемая память не появилась.\n");
    exit(1);
}

void* attach_shm(int fd) {
    void* addr = mmap(NULL, SEGMENT_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    return addr;
}

void detach_shm(void* addr) {
    if (munmap(addr, SEGMENT_SIZE) == -1) {
        perror("munmap");
        exit(1);
    }
}

void remove_shm(int fd) {
    if (close(fd) == -1) {
        perror("close");
    }
    if (shm_unlink(SHM_NAME) == -1) {
        perror("shm_unlink");
    }
}