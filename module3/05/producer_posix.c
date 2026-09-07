#include "common_posix.h"

int main() {
    srand(time(NULL));

    // Создаём семафор и разделяемую память
    sem_t* sem = create_semaphore();
    int fd = create_shm();

    // Подключаемся к разделяемой памяти
    void* shm_addr = attach_shm(fd);
    struct header* hdr = (struct header*)shm_addr;

    // Инициализация заголовка
    hdr->total_size = SEGMENT_SIZE;
    hdr->free_offset = sizeof(struct header);
    hdr->producer_done = 0;
    hdr->all_done = 0;

    int prev_offset = 0;
    int block_count = 0;

    printf("Производитель начал работу.\n");

    while (1) {
        int count = rand() % 10 + 1;
        size_t needed = sizeof(struct block) + count * sizeof(int);

        sem_lock(sem);

        if (hdr->free_offset + needed > hdr->total_size) {
            hdr->producer_done = 1;
            sem_unlock(sem);
            printf("Производитель: достигнут конец памяти. Всего блоков: %d\n", block_count);
            break;
        }

        struct block* blk = (struct block*)((char*)shm_addr + hdr->free_offset);
        blk->id = block_count + 1;
        blk->count = count;
        blk->next = 0;

        for (int i = 0; i < count; i++) {
            blk->data[i] = rand() % 1000;
        }

        if (prev_offset != 0) {
            struct block* prev = (struct block*)((char*)shm_addr + prev_offset);
            prev->next = hdr->free_offset;
        }

        prev_offset = hdr->free_offset;
        hdr->free_offset += needed;
        block_count++;

        sem_unlock(sem);
    }

    printf("Производитель: ожидание завершения потребителей...\n");

    while (1) {
        sem_lock(sem);
        int done = hdr->all_done;
        sem_unlock(sem);

        if (done) break;
        sleep(2);
    }

    printf("Производитель: все потребители завершились, удаляю семафор и разделяемую память.\n");
    remove_semaphore(sem);
    remove_shm(fd);

    detach_shm(shm_addr);
    printf("Производитель завершил работу.\n");
    return 0;
}