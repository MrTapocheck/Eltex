#ifndef LIST_INTERNAL_H
#define LIST_INTERNAL_H

#include "needed.h"

#define DATA_FILE	"contacts.dat"

struct contact_node {
	struct contact data;
	struct contact_node *prev;
	struct contact_node *next;
};

struct contact_table {
	struct contact_node *head;
	struct contact_node *tail;
	unsigned int count;
	unsigned int next_id;
};

void list_free(struct contact_table *table);
struct contact_node *find_node(struct contact_table *table, unsigned int id);
int list_append(struct contact_table *table, const struct contact *c);

#endif
