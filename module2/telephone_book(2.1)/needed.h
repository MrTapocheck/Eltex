#ifndef NEEDED_H
#define NEEDED_H

#define TABLE_SIZE	64
#define MAX_NAME_LEN	32
#define MAX_FIELD_LEN	64
#define MAX_EMAIL_LEN	320
#define ARR_SIZE	4
#define DATA_FILE	"contacts.dat"

struct contact {
	unsigned int id;
	char surname[MAX_NAME_LEN];
	char name[MAX_NAME_LEN];
	char patronymic[MAX_NAME_LEN];
	char workplace[MAX_FIELD_LEN];
	char degree[MAX_FIELD_LEN];
	char phones[ARR_SIZE][MAX_FIELD_LEN];
	char emails[ARR_SIZE][MAX_EMAIL_LEN];
	char social[ARR_SIZE][MAX_FIELD_LEN];
	char messengers[ARR_SIZE][MAX_FIELD_LEN];
};

struct contact_table {
	struct contact contacts[TABLE_SIZE];
	unsigned int count;
	unsigned int next_id;
};

void contact_table_init(struct contact_table *table);
void contact_clear(struct contact *c);
int contact_add(struct contact_table *table, struct contact *c);
struct contact *contact_get(struct contact_table *table, unsigned int id);
int contact_delete(struct contact_table *table, unsigned int id);
unsigned int contact_search(struct contact_table *table,
			    const char *surname, const char *name,
			    struct contact *buf, unsigned int max);
unsigned int contact_sorted_copy(const struct contact_table *table,
				 struct contact *buf);
int contact_save(const struct contact_table *table);
int contact_load(struct contact_table *table);

#endif
