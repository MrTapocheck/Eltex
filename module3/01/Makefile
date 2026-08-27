CC      = gcc
CFLAGS  = -Wall -Wextra -Wpedantic -std=c11 -g
TARGET  = filecopy
SRC     = main.c

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $<

test: $(TARGET)
	@echo " Запуск тестов"
	@echo ""
	@echo "[1/6] Один файл"
	@echo "Команда: ./$(TARGET) test1.txt"
	@./$(TARGET) test1.txt
	@echo "Исходный файл (test1.txt):"
	@cat test1.txt
	@echo "Скопированный файл (test1.txt.copy):"
	@cat test1.txt.copy
	@echo "Ожидаемый результат: содержимое совпадает с исходным"
	@echo ""
	@echo "[2/6] Несколько файлов"
	@echo "Команда: ./$(TARGET) test1.txt test2.txt"
	@./$(TARGET) test1.txt test2.txt
	@echo "Исходный test1.txt:"
	@cat test1.txt
	@echo "Скопированный test1.txt.copy:"
	@cat test1.txt.copy
	@echo "Исходный test2.txt:"
	@cat test2.txt
	@echo "Скопированный test2.txt.copy:"
	@cat test2.txt.copy
	@echo "Ожидаемый результат: каждая копия совпадает с оригиналом"
	@echo ""
	@echo "[3/6] Именованный канал"
	@echo "Команда: ./$(TARGET) -p test_fifo test1.txt"
	@rm -f test_fifo
	@./$(TARGET) -p test_fifo test1.txt
	@echo "Исходный файл (test1.txt):"
	@cat test1.txt
	@echo "Скопированный файл (test1.txt.copy):"
	@cat test1.txt.copy
	@echo "Ожидаемый результат: содержимое совпадает с исходным"
	@test ! -e test_fifo && echo "FIFO удалён: ОК"
	@echo ""
	@echo "[4/6] Именованный канал + несколько файлов"
	@echo "Команда: ./$(TARGET) -p test_fifo2 test1.txt test2.txt"
	@rm -f test_fifo2
	@./$(TARGET) -p test_fifo2 test1.txt test2.txt
	@echo "Исходный test1.txt:"
	@cat test1.txt
	@echo "Скопированный test1.txt.copy:"
	@cat test1.txt.copy
	@echo "Исходный test2.txt:"
	@cat test2.txt
	@echo "Скопированный test2.txt.copy:"
	@cat test2.txt.copy
	@echo "Ожидаемый результат: каждая копия совпадает с оригиналом"
	@test ! -e test_fifo2 && echo "FIFO удалён: ОК"
	@echo ""
	@echo "[5/6] Без аргументов (обработка ошибок)"
	@echo "Команда: ./$(TARGET)"
	@./$(TARGET) >/dev/null 2>error1.txt || true
	@echo "Вывод ошибки:"
	@cat error1.txt
	@echo "Ожидаемый результат: сообщение об использовании программы"
	@echo ""
	@echo "[6/6] -p без имени (обработка ошибок)"
	@echo "Команда: ./$(TARGET) -p"
	@./$(TARGET) -p >/dev/null 2>error2.txt || true
	@echo "Вывод ошибки:"
	@cat error2.txt
	@echo "Ожидаемый результат: сообщение о необходимости указать имя канала"
	@echo ""
	@echo " Тесты завершены"

clean:
	rm -rf $(TARGET) *.copy test_fifo test_fifo2 error1.txt error2.txt