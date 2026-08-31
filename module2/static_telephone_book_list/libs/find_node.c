#include "list_internal.h"

#include <stddef.h>

struct contact_node *find_node(struct contact_table *table, unsigned int id)
{
	struct contact_node *node;

	if (!table)
		return NULL;

	for (node = table->head; node; node = node->next) {
		if (node->data.id == id)
			return node;
	}
	return NULL;
}
