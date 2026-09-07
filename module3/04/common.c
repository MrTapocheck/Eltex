#include "common.h"

// ---------------------- Семафоры -------------------------

int create_semaphore() {
    int semid = semget(SEM_KEY, 1, IPC_CREAT | IPC_EXCL | 0666);
    if (semid == -1) {
        if (errno == EEXIST) {
            fprintf(stderr, "Семафор уже существует. Запустите producer первым.\n");
            exit(1);
        } else {
            perror("semget");
            exit(1);
        }
    }
    // Инициализация семафора значением 1 (мьютекс)
    if (semctl(semid, 0, SETVAL, 1) == -1) {
        perror("semctl SETVAL");
        exit(1);
    }
    return semid;
}

int get_semaphore() {
    int semid;
    // Ожидаем появления семафора (до 30 секунд)
    for (int i = 0; i < 30; i++) {
        semid = semget(SEM_KEY, 1, 0666);
        if (semid != -1) return semid;
        if (errno != ENOENT) {
            perror("semget");
            exit(1);
        }
        sleep(1);
    }
    fprintf(stderr, "Семафор не появился.\n");
    exit(1);
}

void sem_lock(int semid) {
    struct sembuf sb = {0, -1, 0};
    if (semop(semid, &sb, 1) == -1) {
        perror("semop lock");
        exit(1);
    }
}

void sem_unlock(int semid) {
    struct sembuf sb = {0, 1, 0};
    if (semop(semid, &sb, 1) == -1) {
        perror("semop unlock");
        exit(1);
    }
}

void remove_semaphore(int semid) {
    if (semctl(semid, 0, IPC_RMID) == -1) {
        perror("semctl IPC_RMID");
        // некритично
    }
}

// ------------------- Разделяемая память -------------------

int create_shm() {
    int shmid = shmget(SHM_KEY, SEGMENT_SIZE, IPC_CREAT | IPC_EXCL | 0666);
    if (shmid == -1) {
        if (errno == EEXIST) {
            fprintf(stderr, "Разделяемая память уже существует. Запустите producer первым.\n");
            exit(1);
        } else {
            perror("shmget");
            exit(1);
        }
    }
    return shmid;
}

int get_shm() {
    int shmid;
    for (int i = 0; i < 30; i++) {
        shmid = shmget(SHM_KEY, SEGMENT_SIZE, 0666);
        if (shmid != -1) return shmid;
        if (errno != ENOENT) {
            perror("shmget");
            exit(1);
        }
        sleep(1);
    }
    fprintf(stderr, "Разделяемая память не появилась.\n");
    exit(1);
}

void* attach_shm(int shmid) {
    void* addr = shmat(shmid, NULL, 0);
    if (addr == (void*)-1) {
        perror("shmat");
        exit(1);
    }
    return addr;
}

void detach_shm(void* addr) {
    if (shmdt(addr) == -1) {
        perror("shmdt");
        exit(1);
    }
}

void remove_shm(int shmid) {
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl IPC_RMID");
    }
}