#include "needed.h"

#include <dirent.h>
#include <dlfcn.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void flush_line(void)
{
	int ch;

	while ((ch = getchar()) != '\n' && ch != EOF)
		;
}

static int parse_lib_name(const char *filename, char *sym, int sym_size)
{
	const char *base = filename;
	const char *dot;
	int len;

	if (strncmp(filename, "lib", 3) == 0)
		base = filename + 3;

	dot = strrchr(base, '.');
	if (!dot || strcmp(dot, ".so") != 0)
		return -1;

	len = dot - base;
	if (len <= 0 || len >= sym_size)
		return -1;

	memcpy(sym, base, len);
	sym[len] = '\0';
	return 0;
}

static int load_library(struct operation_table *table, const char *path,
			const char *sym)
{
	void *handle;
	operation_func func;
	struct operation *op;

	if (table->count >= MAX_OPS)
		return -1;

	handle = dlopen(path, RTLD_LAZY);
	if (!handle) {
		fprintf(stderr, "Ошибка загрузки библиотеки %s: %s\n",
			path, dlerror());
		return -1;
	}

	func = (operation_func)dlsym(handle, sym);
	if (!func) {
		fprintf(stderr, "Ошибка получения функции %s: %s\n",
			sym, dlerror());
		dlclose(handle);
		return -1;
	}

	op = &table->ops[table->count++];
	strncpy(op->sym, sym, sizeof(op->sym) - 1);
	op->sym[sizeof(op->sym) - 1] = '\0';
	strncpy(op->name, operation_title(sym), sizeof(op->name) - 1);
	op->name[sizeof(op->name) - 1] = '\0';
	op->func = func;
	op->handle = handle;
	return 0;
}

static int load_libraries(struct operation_table *table)
{
	DIR *dir;
	struct dirent *entry;
	char path[512];
	char sym[MAX_NAME_LEN];

	dir = opendir(LIBS_DIR);
	if (!dir) {
		fprintf(stderr, "Не удалось открыть каталог %s\n", LIBS_DIR);
		return -1;
	}

	while ((entry = readdir(dir)) != NULL) {
		if (parse_lib_name(entry->d_name, sym, sizeof(sym)) < 0)
			continue;
		snprintf(path, sizeof(path), "%s/%s", LIBS_DIR, entry->d_name);
		load_library(table, path, sym);
	}

	closedir(dir);
	return table->count > 0 ? 0 : -1;
}

static void unload_libraries(struct operation_table *table)
{
	int i;

	for (i = 0; i < table->count; i++) {
		if (table->ops[i].handle)
			dlclose(table->ops[i].handle);
	}
	table->count = 0;
}

static void show_menu(const struct operation_table *table)
{
	int i;

	printf("\nДоступные операции:\n");
	for (i = 0; i < table->count; i++)
		printf("%d - %s (%s)\n", i + 1, table->ops[i].name,
		       table->ops[i].sym);
	printf("0 - выход\n");
}

static int read_choice(int *choice)
{
	printf("Выбор: ");
	if (scanf("%d", choice) == 1) {
		flush_line();
		return 0;
	}
	flush_line();
	return feof(stdin) ? -2 : -1;
}

static int read_args(double *values, int *count)
{
	int num;
	int i;

	printf("Количество аргументов (1-%d): ", MAX_ARGS);
	if (scanf("%d", &num) != 1 || num < 1 || num > MAX_ARGS) {
		flush_line();
		printf("Некорректное количество аргументов.\n");
		return -1;
	}
	flush_line();

	printf("Введите %d чисел(а): ", num);
	for (i = 0; i < num; i++) {
		if (scanf("%lf", &values[i]) != 1) {
			flush_line();
			printf("Ошибка ввода аргументов.\n");
			return -1;
		}
	}
	flush_line();

	*count = num;
	return 0;
}

static double call_operation(operation_func func, int count,
			     const double *values)
{
	if (!func || count < 1)
		return NAN;

	return func(count, values);
}

const char *operation_title(const char *sym)
{
	if (strcmp(sym, "sum") == 0)
		return "Сложение";
	if (strcmp(sym, "multiply") == 0)
		return "Умножение";
	if (strcmp(sym, "max") == 0)
		return "Максимум";
	if (strcmp(sym, "min") == 0)
		return "Минимум";
	if (strcmp(sym, "divide") == 0)
		return "Деление";

	return sym;
}

int main(void)
{
	struct operation_table table = { 0 };
	double values[MAX_ARGS];
	double result;
	int choice;
	int count;

	setlocale(LC_ALL, "Russian");

	if (load_libraries(&table) < 0) {
		fprintf(stderr, "Не найдено ни одной библиотеки в %s\n",
			LIBS_DIR);
		return 1;
	}

	for (;;) {
		show_menu(&table);
		switch (read_choice(&choice)) {
		case -2:
			goto done;
		case -1:
			continue;
		default:
			break;
		}
		if (choice == 0)
			goto done;
		if (choice < 1 || choice > table.count) {
			printf("Некорректный выбор.\n");
			continue;
		}

		if (read_args(values, &count) < 0)
			continue;

		result = call_operation(table.ops[choice - 1].func, count,
					values);
		if (isnan(result))
			printf("Операция не выполнена.\n");
		else
			printf("Результат = %.6g\n", result);
	}

done:
	unload_libraries(&table);
	return 0;
}
