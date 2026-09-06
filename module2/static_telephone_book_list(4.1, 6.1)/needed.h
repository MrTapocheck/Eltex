#ifndef NEEDED_H
#define NEEDED_H

#define TABLE_SIZE	64
#define MAX_NAME_LEN	32
#define MAX_FIELD_LEN	64
#define MAX_EMAIL_LEN	320
#define ARR_SIZE	4

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

typedef struct contact_book contact_book;

contact_book *contact_book_create(void);
void contact_book_free(contact_book *book);
unsigned int contact_book_count(const contact_book *book);

void contact_clear(struct contact *c);
int contact_add(contact_book *book, struct contact *c);
struct contact *contact_get(contact_book *book, unsigned int id);
int contact_delete(contact_book *book, unsigned int id);
unsigned int contact_search(contact_book *book,
			    const char *surname, const char *name,
			    struct contact *buf, unsigned int max);
unsigned int contact_sorted_copy(const contact_book *book,
				 struct contact *buf);
int contact_save(const contact_book *book);
int contact_load(contact_book *book);

#endif
