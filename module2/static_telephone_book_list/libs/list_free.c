#include "list_internal.h"

#include <stdlib.h>

void list_free(struct contact_table *table)
{
	struct contact_node *node;
	struct contact_node *next;

	if (!table)
		return;

	for (node = table->head; node; node = next) {
		next = node->next;
		free(node);
	}
	table->head = NULL;
	table->tail = NULL;
	table->count = 0;
}
