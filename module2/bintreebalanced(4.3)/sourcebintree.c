#include "needed.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BALANCE_THRESHOLD 10

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

static void consume_line(FILE *fp)
{
	int ch;
	while ((ch = fgetc(fp)) != '\n' && ch != EOF)
		;
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

static int tree_height(struct tree_node *node)
{
	return node ? node->height : 0;
}

static int balance_factor(struct tree_node *node)
{
	if (!node) return 0;
	return tree_height(node->left) - tree_height(node->right);
}

static void update_height(struct tree_node *node)
{
	if (node) {
		int left_h = tree_height(node->left);
		int right_h = tree_height(node->right);
		node->height = (left_h > right_h ? left_h : right_h) + 1;
	}
}

static struct tree_node *rotate_right(struct tree_node *y)
{
	struct tree_node *x = y->left;
	struct tree_node *T2 = x->right;

	x->right = y;
	y->left = T2;

	update_height(y);
	update_height(x);
	return x;
}

static struct tree_node *rotate_left(struct tree_node *x)
{
	struct tree_node *y = x->right;
	struct tree_node *T2 = y->left;

	y->left = x;
	x->right = T2;

	update_height(x);
	update_height(y);
	return y;
}

static struct tree_node *balance_tree(struct tree_node *node)
{
	if (!node) return NULL;

	update_height(node);
	int bf = balance_factor(node);

	if (bf > 1) {
		if (balance_factor(node->left) < 0)
			node->left = rotate_left(node->left);
		return rotate_right(node);
	}

	if (bf < -1) {
		if (balance_factor(node->right) > 0)
			node->right = rotate_right(node->right);
		return rotate_left(node);
	}

	return node;
}

static struct tree_node *insert_node(struct tree_node *node, struct contact *c, unsigned int *count)
{
	if (!node) {
		struct tree_node *new_node = malloc(sizeof(struct tree_node));
		if (!new_node) return NULL;
		new_node->data = *c;
		new_node->left = NULL;
		new_node->right = NULL;
		new_node->height = 1;
		(*count)++;
		return new_node;
	}

	int cmp = strcmp(c->surname, node->data.surname);
	if (cmp == 0)
		cmp = strcmp(c->name, node->data.name);

	if (cmp < 0)
		node->left = insert_node(node->left, c, count);
	else if (cmp > 0)
		node->right = insert_node(node->right, c, count);
	else
		return node;

	return balance_tree(node);
}

static struct tree_node *find_min(struct tree_node *node)
{
	if (!node) return NULL;
	while (node->left)
		node = node->left;
	return node;
}

static struct tree_node *delete_min(struct tree_node *node)
{
	if (!node) return NULL;
	if (!node->left) {
		struct tree_node *right = node->right;
		free(node);
		return right;
	}
	node->left = delete_min(node->left);
	return balance_tree(node);
}

static struct tree_node *delete_node(struct tree_node *node, unsigned int id, int *deleted)
{
	if (!node) return NULL;

	if (id < node->data.id)
		node->left = delete_node(node->left, id, deleted);
	else if (id > node->data.id)
		node->right = delete_node(node->right, id, deleted);
	else {
		*deleted = 1;
		if (!node->left) {
			struct tree_node *right = node->right;
			free(node);
			return right;
		}
		if (!node->right) {
			struct tree_node *left = node->left;
			free(node);
			return left;
		}
		struct tree_node *min_node = find_min(node->right);
		node->data = min_node->data;
		node->right = delete_min(node->right);
	}

	return balance_tree(node);
}

static struct contact *search_by_id(struct tree_node *node, unsigned int id)
{
	if (!node) return NULL;

	if (id < node->data.id)
		return search_by_id(node->left, id);
	else if (id > node->data.id)
		return search_by_id(node->right, id);
	else
		return &node->data;
}

static void collect_sorted(struct tree_node *node, struct contact *buf, unsigned int *index)
{
	if (!node) return;
	collect_sorted(node->left, buf, index);
	buf[(*index)++] = node->data;
	collect_sorted(node->right, buf, index);
}

static struct tree_node *build_balanced_tree(struct contact *arr, int start, int end, unsigned int *count)
{
	if (start > end) return NULL;

	int mid = (start + end) / 2;
	struct tree_node *node = malloc(sizeof(struct tree_node));
	if (!node) return NULL;

	node->data = arr[mid];
	node->left = build_balanced_tree(arr, start, mid - 1, count);
	node->right = build_balanced_tree(arr, mid + 1, end, count);
	update_height(node);
	(*count)++;
	return node;
}

void free_tree(struct tree_node *node)
{
	if (!node) return;
	free_tree(node->left);
	free_tree(node->right);
	free(node);
}

void balance_whole_tree(struct contact_table *table)
{
	if (!table->root || table->count < 2) return;

	struct contact *buf = malloc(table->count * sizeof(struct contact));
	if (!buf) return;

	unsigned int idx = 0;
	collect_sorted(table->root, buf, &idx);

	free_tree(table->root);
	table->root = NULL;
	table->count = 0;

	table->root = build_balanced_tree(buf, 0, idx - 1, &table->count);
	free(buf);
	table->modifications_since_balance = 0;
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

void contact_table_init(struct contact_table *table)
{
	table->root = NULL;
	table->count = 0;
	table->next_id = 1;
	table->modifications_since_balance = 0;
}

void contact_clear(struct contact *c)
{
	if (c)
		memset(c, 0, sizeof(*c));
}

int contact_add(struct contact_table *table, struct contact *c)
{
	if (!table || !c) return -1;
	if (!c->surname[0] || !c->name[0]) return -1;

	c->id = table->next_id++;
	table->root = insert_node(table->root, c, &table->count);
	if (!table->root) return -1;

	table->modifications_since_balance++;
	if (table->modifications_since_balance >= BALANCE_THRESHOLD)
		balance_whole_tree(table);

	return 0;
}

struct contact *contact_get(struct contact_table *table, unsigned int id)
{
	if (!table) return NULL;
	return search_by_id(table->root, id);
}

int contact_delete(struct contact_table *table, unsigned int id)
{
	if (!table) return -1;

	int deleted = 0;
	table->root = delete_node(table->root, id, &deleted);

	if (deleted) {
		table->count--;
		table->modifications_since_balance++;
		if (table->modifications_since_balance >= BALANCE_THRESHOLD)
			balance_whole_tree(table);
		return 0;
	}
	return -1;
}

unsigned int contact_search(struct contact_table *table,
			    const char *surname, const char *name,
			    struct contact *buf, unsigned int max)
{
	if (!table || !buf || max == 0) return 0;
	if ((!surname || !surname[0]) && (!name || !name[0])) return 0;

	struct contact *all = malloc(table->count * sizeof(struct contact));
	if (!all) return 0;

	unsigned int idx = 0;
	collect_sorted(table->root, all, &idx);

	unsigned int n = 0;
	for (unsigned int i = 0; i < table->count && n < max; i++) {
		struct contact *c = &all[i];

		if (surname && surname[0] && !str_eq(c->surname, surname))
			continue;
		if (name && name[0] && !str_eq(c->name, name))
			continue;
		buf[n++] = *c;
	}

	free(all);
	return n;
}

unsigned int contact_sorted_copy(const struct contact_table *table,
				 struct contact *buf)
{
	if (!table || !buf || !table->count) return 0;

	unsigned int idx = 0;
	collect_sorted(table->root, buf, &idx);
	return idx;
}

int contact_save(const struct contact_table *table)
{
	FILE *fp;
	unsigned int i;

	if (!table) return -1;

	fp = fopen(DATA_FILE, "w");
	if (!fp) return -1;

	if (fprintf(fp, "%u\n%u\n", table->count, table->next_id) < 0)
		goto err;

	struct contact *buf = malloc(table->count * sizeof(struct contact));
	if (!buf) goto err;

	unsigned int idx = 0;
	collect_sorted(table->root, buf, &idx);

	for (i = 0; i < table->count; i++) {
		if (contact_write(fp, &buf[i]) < 0) {
			free(buf);
			goto err;
		}
	}

	free(buf);
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

	if (!table) return -1;

	fp = fopen(DATA_FILE, "r");
	if (!fp) return -1;

	if (read_file_line(fp, buf, sizeof(buf)) < 0)
		goto err;
	count = (unsigned int)atoi(buf);

	if (read_file_line(fp, buf, sizeof(buf)) < 0)
		goto err;

	free_tree(table->root);
	contact_table_init(table);
	table->next_id = (unsigned int)atoi(buf);

	for (i = 0; i < count; i++) {
		struct contact c;
		if (contact_read_file(fp, &c) < 0)
			goto err;

		table->root = insert_node(table->root, &c, &table->count);
		if (!table->root) goto err;
	}

	fclose(fp);
	balance_whole_tree(table);
	return 0;

err:
	fclose(fp);
	free_tree(table->root);
	contact_table_init(table);
	return -1;
}