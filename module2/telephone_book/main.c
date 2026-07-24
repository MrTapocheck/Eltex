#include "needed.h"

#include <stdio.h>
#include <string.h>

static void flush_line(void)
{
	int ch;

	while ((ch = getchar()) != '\n' && ch != EOF)
		;
}

static void str_trim(char *s)
{
	int end;

	while (*s == ' ' || *s == '\t') {
		memmove(s, s + 1, strlen(s));
	}
	end = strlen(s) - 1;
	while (end >= 0 && (s[end] == ' ' || s[end] == '\t'))
		s[end--] = '\0';
}

static void read_line(const char *prompt, char *buf, int size)
{
	char *nl;

	if (prompt)
		printf("%s", prompt);
	if (!fgets(buf, size, stdin)) {
		buf[0] = '\0';
		return;
	}
	nl = strchr(buf, '\n');
	if (nl)
		*nl = '\0';
	else
		flush_line();
	str_trim(buf);
}

static void read_arr(const char *label, char *base, int elem_size)
{
	int i;
	char prompt[48];

	for (i = 0; i < ARR_SIZE; i++) {
		snprintf(prompt, sizeof(prompt), "%s %d: ", label, i + 1);
		read_line(prompt, base + i * elem_size, elem_size);
	}
}

static void show_field(const char *label, const char *value)
{
	if (value && value[0])
		printf("  %s: %s\n", label, value);
}

static void show_arr(const char *label, const char *base, int elem_size)
{
	int i;
	const char *val;

	for (i = 0; i < ARR_SIZE; i++) {
		val = base + i * elem_size;
		if (val[0])
			printf("  %s %d: %s\n", label, i + 1, val);
	}
}

static void show_contact(const struct contact *c)
{
	if (!c)
		return;

	printf("Контакт #%u\n", c->id);
	show_field("Фамилия", c->surname);
	show_field("Имя", c->name);
	show_field("Отчество", c->patronymic);
	show_field("Место работы", c->workplace);
	show_field("Должность", c->degree);
	show_arr("Телефон", c->phones[0], MAX_FIELD_LEN);
	show_arr("Email", c->emails[0], MAX_EMAIL_LEN);
	show_arr("Соцсеть", c->social[0], MAX_FIELD_LEN);
	show_arr("Мессенджер", c->messengers[0], MAX_FIELD_LEN);
	printf("\n");
}

static void show_all(struct contact_table *table)
{
	struct contact sorted[TABLE_SIZE];
	unsigned int n;
	unsigned int i;

	n = contact_sorted_copy(table, sorted);
	if (n == 0) {
		printf("Список пуст.\n");
		return;
	}
	for (i = 0; i < n; i++)
		show_contact(&sorted[i]);
}

static void read_contact(struct contact *c)
{
	contact_clear(c);
	read_line("Фамилия: ", c->surname, sizeof(c->surname));
	read_line("Имя: ", c->name, sizeof(c->name));
	read_line("Отчество: ", c->patronymic, sizeof(c->patronymic));
	read_line("Место работы: ", c->workplace, sizeof(c->workplace));
	read_line("Должность: ", c->degree, sizeof(c->degree));
	read_arr("Телефон", c->phones[0], MAX_FIELD_LEN);
	read_arr("Email", c->emails[0], MAX_EMAIL_LEN);
	read_arr("Соцсеть", c->social[0], MAX_FIELD_LEN);
	read_arr("Мессенджер", c->messengers[0], MAX_FIELD_LEN);
}

static void edit_slot(char *base, int elem_size, const char *label)
{
	int n;

	printf("Номер (1-%d): ", ARR_SIZE);
	if (scanf("%d", &n) != 1)
		return;
	flush_line();
	if (n < 1 || n > ARR_SIZE)
		return;
	read_line(label, base + (n - 1) * elem_size, elem_size);
}

static void action_add(struct contact_table *table)
{
	struct contact c;

	read_contact(&c);
	if (!c.surname[0] || !c.name[0]) {
		printf("Фамилия и имя обязательны.\n");
		return;
	}
	if (contact_add(table, &c) < 0) {
		printf("Не удалось добавить контакт.\n");
		return;
	}
	printf("Контакт добавлен (id=%u).\n", c.id);
}

static void action_edit(struct contact_table *table, unsigned int id)
{
	struct contact *c;
	int field;

	c = contact_get(table, id);
	if (!c) {
		printf("Контакт не найден.\n");
		return;
	}

	printf("1-фамилия\n"
            "2-имя\n"
            "3-отчество\n"
            "4-работа\n"
            "5-должность\n"
	        "6-телефон\n"
            "7-email\n"
            "8-соцсеть\n"
            "9-мессенджер\n"
            "0-отмена\n");
	printf("Поле: ");
	if (scanf("%d", &field) != 1)
		return;
	flush_line();
	if (field == 0)
		return;

	switch (field) {
	case 1:
		read_line("Фамилия: ", c->surname, sizeof(c->surname));
		break;
	case 2:
		read_line("Имя: ", c->name, sizeof(c->name));
		break;
	case 3:
		read_line("Отчество: ", c->patronymic, sizeof(c->patronymic));
		break;
	case 4:
		read_line("Место работы: ", c->workplace, sizeof(c->workplace));
		break;
	case 5:
		read_line("Должность: ", c->degree, sizeof(c->degree));
		break;
	case 6:
		edit_slot(c->phones[0], MAX_FIELD_LEN, "Телефон: ");
		break;
	case 7:
		edit_slot(c->emails[0], MAX_EMAIL_LEN, "Email: ");
		break;
	case 8:
		edit_slot(c->social[0], MAX_FIELD_LEN, "Соцсеть: ");
		break;
	case 9:
		edit_slot(c->messengers[0], MAX_FIELD_LEN, "Мессенджер: ");
		break;
	default:
		printf("Некорректное поле.\n");
		break;
	}
}

static void action_find(struct contact_table *table)
{
	char surname[MAX_NAME_LEN];
	char name[MAX_NAME_LEN];
	struct contact found[TABLE_SIZE];
	unsigned int n;
	unsigned int i;

	read_line("Фамилия (Enter - пропустить): ", surname, sizeof(surname));
	read_line("Имя (Enter - пропустить): ", name, sizeof(name));
	n = contact_search(table, surname, name, found, TABLE_SIZE);
	if (n == 0) {
		printf("Не найден.\n");
		return;
	}
	for (i = 0; i < n; i++)
		show_contact(&found[i]);
}

static void action_delete(struct contact_table *table, unsigned int id)
{
	if (contact_delete(table, id) < 0)
		printf("Контакт не найден.\n");
	else
		printf("Контакт удалён.\n");
}

int main(void)
{
	struct contact_table table;
	int choice;
	unsigned int id;

	contact_table_init(&table);

	for (;;) {
		printf("1-список\n"
		       "2-найти\n"
		       "3-добавить\n"
		       "4-изменить\n"
		       "5-удалить\n"
		       "6-сохранить\n"
		       "7-загрузить\n"
		       "0-выход\n");
		printf("Выбор: ");
		if (scanf("%d", &choice) != 1) {
			flush_line();
			continue;
		}
		flush_line();

		switch (choice) {
		case 1:
			show_all(&table);
			break;
		case 2:
			action_find(&table);
			break;
		case 3:
			action_add(&table);
			break;
		case 4:
			printf("ID: ");
			if (scanf("%u", &id) == 1) {
				flush_line();
				action_edit(&table, id);
			}
			break;
		case 5:
			printf("ID: ");
			if (scanf("%u", &id) == 1) {
				flush_line();
				action_delete(&table, id);
			}
			break;
		case 6:
			if (contact_save(&table) < 0)
				printf("Ошибка сохранения.\n");
			else
				printf("Сохранено.\n");
			break;
		case 7:
			if (contact_load(&table) < 0)
				printf("Ошибка загрузки.\n");
			else
				printf("Загружено: %u\n", table.count);
			break;
		case 0:
			return 0;
		}
	}
}
