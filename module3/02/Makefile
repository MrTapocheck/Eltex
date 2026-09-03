CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=gnu11 -g
TARGET = broker
SRC = main.c

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

test: $(TARGET)
	@echo "1. Запуск брокера в фоне"
	@echo "./$(TARGET) -b > broker.log 2>&1 & echo $$! > broker.pid"
	@./$(TARGET) -b > broker.log 2>&1 & echo $$! > broker.pid
	@sleep 1

	@echo ""
	@echo "2. Запуск подписчика на темы news и sport"
	@echo "./$(TARGET) -s news sport > subscriber.log 2>&1 & echo $$! > subscriber.pid"
	@./$(TARGET) -s news sport > subscriber.log 2>&1 & echo $$! > subscriber.pid
	@sleep 1

	@echo ""
	@echo "3. Отправка сообщения в тему news"
	@echo "./$(TARGET) -p news \"Hello news\""
	@./$(TARGET) -p news "Hello news"
	@sleep 1

	@echo ""
	@echo "4. Отправка сообщения в тему sport"
	@echo "./$(TARGET) -p sport \"Hello sport\""
	@./$(TARGET) -p sport "Hello sport"
	@sleep 1

	@echo ""
	@echo "5. Остановка брокера"
	@echo 'kill -INT $$(cat broker.pid)'
	@kill -INT $$(cat broker.pid) 2>/dev/null || true
	@sleep 1

	@echo ""
	@echo "6. Вывод лога подписчика"
	@echo "cat subscriber.log"
	@cat subscriber.log

	@echo ""
	@echo "7. Вывод лога брокера"
	@echo "cat broker.log"
	@cat broker.log

	@rm -f broker.pid subscriber.pid broker.log subscriber.log

clean:
	rm -f $(TARGET) broker.pid subscriber.pid
	rm -f broker.log subscriber.log