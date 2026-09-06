#include "list_internal.h"

#include <stdlib.h>

int list_append(struct contact_table *table, const struct contact *c)
{
	struct contact_node *node;

	if (!table || !c)
		return -1;
	if (table->count >= TABLE_SIZE)
		return -1;

	node = malloc(sizeof(*node));
	if (!node)
		return -1;

	node->data = *c;
	node->next = NULL;
	node->prev = table->tail;

	if (table->tail)
		table->tail->next = node;
	else
		table->head = node;
	table->tail = node;
	table->count++;
	return 0;
}
