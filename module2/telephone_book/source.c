#include "needed.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void consume_line(FILE *fp)
{
	int ch;

	while ((ch = fgetc(fp)) != '\n' && ch != EOF)
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

static int str_eq(const char *a, const char *b)
{
	char ta[MAX_FIELD_LEN];
	char tb[MAX_FIELD_LEN];

	strncpy(ta, a, sizeof(ta) - 1);
	ta[sizeof(ta) - 1] = '\0';
	strncpy(tb, b, sizeof(tb) - 1);
	tb[sizeof(tb) - 1] = '\0';
	str_trim(ta);
	str_trim(tb);
	return strcmp(ta, tb) == 0;
}

static int read_file_line(FILE *fp, char *buf, int size)
{
	char *nl;

	if (!fgets(buf, size, fp))
		return -1;
	nl = strchr(buf, '\n');
	if (nl)
		*nl = '\0';
	else if (!feof(fp))
		consume_line(fp);
	str_trim(buf);
	return 0;
}

static int write_arr(FILE *fp, const char *base, int elem_size)
{
	int i;

	for (i = 0; i < ARR_SIZE; i++) {
		if (fprintf(fp, "%s\n", base + i * elem_size) < 0)
			return -1;
	}
	return 0;
}

static int read_arr_file(FILE *fp, char *base, int elem_size)
{
	int i;

	for (i = 0; i < ARR_SIZE; i++) {
		if (read_file_line(fp, base + i * elem_size, elem_size) < 0)
			return -1;
	}
	return 0;
}

static int contact_cmp(const void *a, const void *b)
{
	const struct contact *ca = a;
	const struct contact *cb = b;
	int r;

	r = strcmp(ca->surname, cb->surname);
	if (r != 0)
		return r;
	return strcmp(ca->name, cb->name);
}

void contact_clear(struct contact *c)
{
	if (c)
		memset(c, 0, sizeof(*c));
}

void contact_table_init(struct contact_table *table)
{
	table->count = 0;
	table->next_id = 1;
}

int contact_add(struct contact_table *table, struct contact *c)
{
	if (!table || !c)
		return -1;
	if (table->count >= TABLE_SIZE)
		return -1;
	if (!c->surname[0] || !c->name[0])
		return -1;

	c->id = table->next_id++;
	table->contacts[table->count++] = *c;
	return 0;
}

struct contact *contact_get(struct contact_table *table, unsigned int id)
{
	unsigned int i;

	for (i = 0; i < table->count; i++) {
		if (table->contacts[i].id == id)
			return &table->contacts[i];
	}
	return NULL;
}

int contact_delete(struct contact_table *table, unsigned int id)
{
	unsigned int i;

	for (i = 0; i < table->count; i++) {
		if (table->contacts[i].id != id)
			continue;
		for (; i + 1 < table->count; i++)
			table->contacts[i] = table->contacts[i + 1];
		table->count--;
		return 0;
	}
	return -1;
}

unsigned int contact_search(struct contact_table *table,
			    const char *surname, const char *name,
			    struct contact *buf, unsigned int max)
{
	unsigned int i;
	unsigned int n;

	if (!table || !buf || max == 0)
		return 0;
	if ((!surname || !surname[0]) && (!name || !name[0]))
		return 0;

	n = 0;
	for (i = 0; i < table->count && n < max; i++) {
		struct contact *c = &table->contacts[i];

		if (surname && surname[0] &&
		    !str_eq(c->surname, surname))
			continue;
		if (name && name[0] &&
		    !str_eq(c->name, name))
			continue;
		buf[n++] = *c;
	}
	return n;
}

unsigned int contact_sorted_copy(const struct contact_table *table,
				 struct contact *buf)
{
	if (!table || !buf || !table->count)
		return 0;

	memcpy(buf, table->contacts, table->count * sizeof(struct contact));
	qsort(buf, table->count, sizeof(struct contact), contact_cmp);
	return table->count;
}

static int contact_write(FILE *fp, const struct contact *c)
{
	if (fprintf(fp, "%u\n%s\n%s\n%s\n%s\n%s\n",
		    c->id, c->surname, c->name, c->patronymic,
		    c->workplace, c->degree) < 0)
		return -1;
	if (write_arr(fp, c->phones[0], MAX_FIELD_LEN) < 0 ||
	    write_arr(fp, c->emails[0], MAX_EMAIL_LEN) < 0 ||
	    write_arr(fp, c->social[0], MAX_FIELD_LEN) < 0 ||
	    write_arr(fp, c->messengers[0], MAX_FIELD_LEN) < 0)
		return -1;
	return 0;
}

static int contact_read_file(FILE *fp, struct contact *c)
{
	char buf[16];

	if (read_file_line(fp, buf, sizeof(buf)) < 0)
		return -1;
	c->id = (unsigned int)atoi(buf);
	if (read_file_line(fp, c->surname, sizeof(c->surname)) < 0 ||
	    read_file_line(fp, c->name, sizeof(c->name)) < 0 ||
	    read_file_line(fp, c->patronymic, sizeof(c->patronymic)) < 0 ||
	    read_file_line(fp, c->workplace, sizeof(c->workplace)) < 0 ||
	    read_file_line(fp, c->degree, sizeof(c->degree)) < 0)
		return -1;
	if (read_arr_file(fp, c->phones[0], MAX_FIELD_LEN) < 0 ||
	    read_arr_file(fp, c->emails[0], MAX_EMAIL_LEN) < 0 ||
	    read_arr_file(fp, c->social[0], MAX_FIELD_LEN) < 0 ||
	    read_arr_file(fp, c->messengers[0], MAX_FIELD_LEN) < 0)
		return -1;
	return 0;
}

int contact_save(const struct contact_table *table)
{
	FILE *fp;
	unsigned int i;

	if (!table)
		return -1;

	fp = fopen(DATA_FILE, "w");
	if (!fp)
		return -1;

	if (fprintf(fp, "%u\n%u\n", table->count, table->next_id) < 0)
		goto err;

	for (i = 0; i < table->count; i++) {
		if (contact_write(fp, &table->contacts[i]) < 0)
			goto err;
	}

	fclose(fp);
	return 0;

err:
	fclose(fp);
	return -1;
}

int contact_load(struct contact_table *table)
{
	FILE *fp;
	unsigned int count;
	unsigned int i;
	char buf[16];

	if (!table)
		return -1;

	fp = fopen(DATA_FILE, "r");
	if (!fp)
		return -1;

	if (read_file_line(fp, buf, sizeof(buf)) < 0)
		goto err;
	count = (unsigned int)atoi(buf);
	if (count > TABLE_SIZE)
		goto err;
	if (read_file_line(fp, buf, sizeof(buf)) < 0)
		goto err;

	contact_table_init(table);
	table->next_id = (unsigned int)atoi(buf);

	for (i = 0; i < count; i++) {
		if (contact_read_file(fp, &table->contacts[i]) < 0)
			goto err;
	}

	table->count = count;
	fclose(fp);
	return 0;

err:
	fclose(fp);
	contact_table_init(table);
	return -1;
}
