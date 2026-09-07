#include "common.h"

int main() {
    srand(time(NULL) ^ getpid());//чтобы генерировать "некоторое время"

    int semid = get_semaphore();
    int shmid = get_shm();

    // подключаемся к разделяемой памяти
    void* shm_addr = attach_shm(shmid);
    struct header* hdr = (struct header*)shm_addr;

    printf("Потребитель (pid=%d) начал работу.\n", getpid());

    while (1) {
        // захватываем семафор для проверки состояния
        sem_lock(semid);

        // если кто-то сказал, что все блоки прочитаны, завершаемся
        if (hdr->all_done) {
            sem_unlock(semid);
            break;
        }

        // ищем необработанный блок
        int found = 0;
        int offset = sizeof(struct header);
        while (offset != 0) {
            struct block* blk = (struct block*)((char*)shm_addr + offset);
            if (blk->count > 0) {
                int cnt = blk->count;
                int data[cnt];              // копируем данные в локальный массив
                memcpy(data, blk->data, cnt * sizeof(int));

                // помечаем блок как обработанный
                blk->count = 0;

                // освобождаем семафор, чтобы другие могли работать
                sem_unlock(semid);

                // поиск min и max
                int min = data[0], max = data[0];
                for (int i = 1; i < cnt; i++) {
                    if (data[i] < min) min = data[i];
                    if (data[i] > max) max = data[i];
                }

                printf("Потребитель (pid=%d): набор %d из %d элементов, min=%d, max=%d\n",
                       getpid(),blk->id, cnt, min, max);

                found = 1;
                // засыпаем на некоторое время
                usleep(rand() % 1000000);   // 0-1 
                break;
            }
            offset = blk->next;
        }

        if (found) {
            continue;
        }

        // необработанных блоков нет
        if (hdr->producer_done) {
            // Производитель закончил(закончилась ли разделяемая память), все данные обработаны
            hdr->all_done = 1;
            sem_unlock(semid);

            printf("Потребитель (pid=%d): все наборы обработаны\n", getpid());
            break;
        } else {
            // производитель ещё пишет, ждём
            sem_unlock(semid);
            usleep(rand() % 500000);   // 0 – 0.5 секунды
        }
    }

    // Отключаемся от разделяемой памяти
    detach_shm(shm_addr);

    printf("Потребитель (pid=%d) завершил работу.\n", getpid());
    return 0;
}