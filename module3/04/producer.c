#include "common.h"

int main() {
    srand(time(NULL));

    // создаём семафор и разделяемую память
    int semid = create_semaphore();
    int shmid = create_shm();

    // подключаемся к разделяемой памяти
    void* shm_addr = attach_shm(shmid);
    struct header* hdr = (struct header*)shm_addr;

    // инициализация заголовка
    hdr->total_size = SEGMENT_SIZE;
    hdr->free_offset = sizeof(struct header);
    hdr->producer_done = 0;
    hdr->all_done = 0;

    int prev_offset = 0;      // смещение предыдущего блока
    int block_count = 0;

    printf("Производитель начал работу.\n");

    while (1) {
        // генерируем случайное количество элементов от 1 до 10
        int count = rand() % 10 + 1;
        size_t needed = sizeof(struct block) + count * sizeof(int);

        // захватываем семафор для безопасной проверки и записи
        sem_lock(semid);

        if (hdr->free_offset + needed > hdr->total_size) {//закончилась ли разделяемая память
            hdr->producer_done = 1;
            sem_unlock(semid);
            printf("Производитель: достигнут конец памяти. Всего блоков: %d\n", block_count);
            break;
        }

        struct block* blk = (struct block*)((char*)shm_addr + hdr->free_offset);
        blk->id = block_count + 1;
        blk->count = count;
        blk->next = 0;   // пока считаем, что это последний блок

        // рандомные данные
        for (int i = 0; i < count; i++) {
            blk->data[i] = rand() % 1000;
        }

        // если был предыдущий блок, обновляем у него указатель на текущий
        if (prev_offset != 0) {
            struct block* prev = (struct block*)((char*)shm_addr + prev_offset);
            prev->next = hdr->free_offset;
        }

        // сохраняем текущее смещение как предыдущее для следующего блока
        prev_offset = hdr->free_offset;

        hdr->free_offset += needed;
        block_count++;

        sem_unlock(semid);
    }


    printf("Производитель: ожидание завершения потребителей...\n");

    while (1) {
        sem_lock(semid);
        int done = hdr->all_done;
        sem_unlock(semid);

        if (done) {
            break;
        }
        sleep(2);   //ждём пока потребители завершатся
    }

    printf("Производитель: все потребители завершились, удаляю семафор и разделяемую память.\n");
    remove_semaphore(semid);
    remove_shm(shmid);

    detach_shm(shm_addr);
    printf("Производитель завершил работу.\n");
    return 0;
}