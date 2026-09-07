#include "common_posix.h"

int main() {
    srand(time(NULL) ^ getpid());

    // Получаем доступ к семафору и разделяемой памяти
    sem_t* sem = get_semaphore();
    int fd = get_shm();

    // Подключаемся к разделяемой памяти
    void* shm_addr = attach_shm(fd);
    struct header* hdr = (struct header*)shm_addr;

    printf("Потребитель (pid=%d) начал работу.\n", getpid());

    while (1) {
        sem_lock(sem);

        if (hdr->all_done) {
            sem_unlock(sem);
            break;
        }

        int found = 0;
        int offset = sizeof(struct header);
        while (offset != 0) {
            struct block* blk = (struct block*)((char*)shm_addr + offset);
            if (blk->count > 0) {
                int cnt = blk->count;
                int id = blk->id;
                int data[cnt];
                memcpy(data, blk->data, cnt * sizeof(int));

                blk->count = 0;

                sem_unlock(sem);

                int min = data[0], max = data[0];
                for (int i = 1; i < cnt; i++) {
                    if (data[i] < min) min = data[i];
                    if (data[i] > max) max = data[i];
                }

                printf("Потребитель (pid=%d): набор %d из %d элементов, min=%d, max=%d\n",
                       getpid(), id, cnt, min, max);

                found = 1;
                usleep(rand() % 1000000);
                break;
            }
            offset = blk->next;
        }

        if (found) continue;

        if (hdr->producer_done) {
            hdr->all_done = 1;
            sem_unlock(sem);
            printf("Потребитель (pid=%d): все наборы обработаны, отключаюсь.\n", getpid());
            break;
        } else {
            sem_unlock(sem);
            usleep(rand() % 500000);
        }
    }

    detach_shm(shm_addr);
    // Семафор не закрываем, так как производитель удалит его
    printf("Потребитель (pid=%d) завершил работу.\n", getpid());
    return 0;
}