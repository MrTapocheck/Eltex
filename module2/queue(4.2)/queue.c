#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <time.h>

// Структура элемента очереди
typedef struct Node {
    int data;          // Данные элемента
    int priority;      // Приоритет элемента (0-255)
    struct Node* next; // Указатель на следующий элемент
} Node;

// Структура очереди
typedef struct {
    Node* front; // начало очереди
    int count;   // количество элементов очереди
} PriorityQueue;

void initQueue(PriorityQueue* q) {
    q->count = 0;
    q->front = NULL;
}

// Освобождение памяти очереди
void freeQueue(PriorityQueue* q) {
    Node* current = q->front;
    while (current != NULL) {
        Node* temp = current;
        current = current->next;
        free(temp);
    }
    q->front = NULL;
    q->count = 0;
}

// 1) Добавление элемента в очередь (сортировка по убыванию приоритета)
void push(PriorityQueue* q, int data, int priority) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (!newNode) {
        printf("Ошибка выделения памяти!\n");
        return;
    }

    newNode->data = data;
    newNode->priority = priority;
    newNode->next = NULL;

    // Если очередь пуста или новый элемент имеет наивысший приоритет
    if (q->front == NULL || q->front->priority < priority) {
        newNode->next = q->front;
        q->front = newNode;
    } else {
        // Ищем место для вставки, чтобы сохранить сортировку по убыванию
        Node* current = q->front;
        Node* prev = NULL;

        // Пропускаем элементы с приоритетом БОЛЬШЕ или РАВНЫМ новому (для стабильности сортировки)
        while (current != NULL && current->priority >= priority) {
            prev = current;
            current = current->next;
        }

        prev->next = newNode;
        newNode->next = current;
    }
    q->count++;
}

// 2) Извлечение элемента из начала очереди (наивысший приоритет)
int pop(PriorityQueue* q, int* data, int* priority) {
    if (q->front == NULL) {
        return 0; // Очередь пуста
    }

    Node* temp = q->front;
    *data = temp->data;
    *priority = temp->priority;

    q->front = q->front->next;
    free(temp);
    q->count--;
    return 1;
}

// 3) Извлечение элемента с указанным приоритетом
int popWithPriority(PriorityQueue* q, int targetPriority, int* data) {
    if (q->front == NULL) {
        return 0;
    }

    Node* current = q->front;
    Node* prev = NULL;

    // Поиск первого элемента с нужным приоритетом
    while (current != NULL && current->priority != targetPriority) {
        prev = current;
        current = current->next;
    }

    if (current == NULL) {
        printf("Элемент с приоритетом %d не найден\n", targetPriority);
        return 0;
    }

    *data = current->data;

    if (prev == NULL) {
        q->front = current->next;
    } else {
        prev->next = current->next;
    }

    free(current);
    q->count--;
    return 1;
}

// 4) Извлечение элемента с приоритетом не ниже заданного (>= minPriority)
int popWithMinPriority(PriorityQueue* q, int minPriority, int* data, int* foundElementPriority) {
    if (q->front == NULL) {
        return 0;
    }

    Node* current = q->front;
    Node* prev = NULL;

    // поиск элемента с нужным приоритетом
    while (current != NULL && current->priority < minPriority) {
        prev = current;
        current = current->next;
    }

    if (current == NULL) {
        printf("Элемент с приоритетом не ниже %d не найден\n", minPriority);
        return 0;
    }

    *data = current->data;
    *foundElementPriority = current->priority;

    if (prev == NULL) {
        q->front = current->next;
    } else {
        prev->next = current->next;
    }

    free(current);
    q->count--;
    return 1;
}

// Функция для печати очереди
void printQueue(const PriorityQueue* q) {
    if (q->front == NULL) {
        printf("Очередь пуста.\n\n");
        return;
    }
    
    const Node* current = q->front;
    printf("Очередь (всего элементов: %d):\n", q->count);
    while (current != NULL) {
        printf(" -> [Данные: %d, Приоритет: %d]\n", current->data, current->priority);
        current = current->next;
    }
    printf(" -----------------------------------\n\n");
}

// Имитация генерации сообщений
void fillqueue(PriorityQueue* q, int N) {
    printf("Генерация %d сообщений...\n", N);
    for (int i = 1; i <= N; i++) {
        // Данные: номер сообщения * 10, Приоритет: случайное число от 0 до 255
        push(q, i * 10, rand() % 256);
    }
    printf("Генерация завершена.\n\n");
}

int main() {
    setlocale(LC_ALL, "Russian");
    srand((unsigned int)time(NULL));

    PriorityQueue q;
    initQueue(&q);

    while (1) {
        printf("Меню:\n");
        printf("1. Заполнить очередь N элементами\n");
        printf("2. Извлечь первый элемент (наивысший приоритет)\n");
        printf("3. Извлечь элемент с заданным приоритетом\n");
        printf("4. Извлечь элемент с приоритетом не ниже заданного\n");
        printf("5. Вывести очередь\n");
        printf("6. Выйти из программы\n");
        printf("> ");
        
        int choice, N, data, priority, foundPriority;
        
        if (scanf(" %d", &choice) != 1) {
            printf("Некорректный ввод. Попробуйте снова.\n");
            // Очищаем буфер ввода
            while (getchar() != '\n');
            continue;
        }

        switch (choice) {
            case 1:
                printf("Сколько сообщений сгенерировать? ");
                if (scanf(" %d", &N) == 1 && N > 0) {
                    fillqueue(&q, N);
                    printQueue(&q);
                } else {
                    printf("Некорректное число.\n");
                    while (getchar() != '\n');
                }
                break;
            case 2:
                if (pop(&q, &data, &priority)) {
                    printf("Успешно извлечен из начала: сообщение №%d (приоритет %d)\n\n", data, priority);
                } else {
                    printf("Очередь пуста!\n\n");
                }
                break;
            case 3:
                printf("Введите приоритет для поиска (0-255): ");
                if (scanf(" %d", &priority) == 1) {
                    if (popWithPriority(&q, priority, &data)) {
                        printf("Успешно извлечен: сообщение №%d с приоритетом %d\n\n", data, priority);
                    }
                }
                break;
            case 4:
                printf("Введите минимальный порог приоритета (0-255): ");
                if (scanf(" %d", &priority) == 1) {
                    if (popWithMinPriority(&q, priority, &data, &foundPriority)) {
                        printf("Успешно извлечен: сообщение №%d с приоритетом %d (>= %d)\n\n", data, foundPriority, priority);
                    }
                }
                break;
            case 5:
                printQueue(&q);
                break;
            case 6:
                freeQueue(&q); // Освобождаем память перед выходом
                printf("Выход из программы.\n");
                return 0;
            default:
                printf("Неверный пункт меню. Попробуйте снова.\n\n");
                break;
        }
    }
}