#include "array.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define ALIGN(n) (((n) + 7) & ~7)

struct array_node {
	struct array_node *next;
};

struct array {
	int n;
	int size;
	char *data;
	struct array_node *freelist;
};

void block_init(struct block *b, void *data, int size) {
	b->data = (char *)data;
	b->size = size;
}

void *block_slice(struct block *b, int size) {
	void *p;
	if (b->size < size) {
		return 0;
	}
	p = b->data;
	b->data += size;
	b->size -= size;
	return p;
}

struct array *array_new(struct block *b, int n, int size) {
	int data_size;
	void *data;
	char *p;
	int i;
	struct array *a;

	size = ALIGN(size);
	data_size = n * size;
	data = block_slice(b, sizeof(struct array));
	a = (struct array *)data;
	data = block_slice(b, data_size);
	p = (char *)data;
	for (i = 0; i < n - 1; i++) {
		struct array_node *node = (struct array_node *)(p + i*size);
		struct array_node *next = (struct array_node *)(p + (i + 1)*size);
		node->next = next;
	}
	((struct array_node *)(p + (n - 1)*size))->next = 0;
	a->n = n;
	a->size = size;
	a->data = (char *)data;
	a->freelist = (struct array_node *)data;
	return a;
}

int array_size(int n, int size) {
	size = ALIGN(size);
	return n * size + sizeof(struct array);
}

void *array_add(struct array *a) {
	struct array_node *node = a->freelist;
	if (!node) {
		return 0;
	}
	a->freelist = node->next;
	memset(node, 0, a->size);
	return node;
}

void array_rem(struct array *a, void *p) {
	struct array_node *node = (struct array_node *)p;
	if (node) {
		node->next = a->freelist;
		a->freelist = node;
	}
}

int array_id(struct array *a, void *p) {
	int id;
	if (!p) {
		return 0;
	}
	id = ((char *)p - a->data) / a->size;
	assert(id >= 0 && id < a->n);
	return id + 1;
}

void *array_ref(struct array *a, int id) {
	if (!id) {
		return 0;
	}
	id--;
	assert(id >= 0 && id < a->n);
	return a->data + a->size * id;
}

#ifdef _MSC_VER
#include <malloc.h>
#endif
void array_free(struct array *a, void(*_free)(void *p, void *ud), void *ud) {
	int i;
#if defined(_MSC_VER)
	char *flag = _alloca(a->n*sizeof(char));
#else
	char flag[a->n];
#endif
	struct array_node *node = a->freelist;
	memset(flag, 0, a->n*sizeof(char));
	while (node) {
		int idx = array_id(a, node) - 1;
		flag[idx] = 1;
		node = node->next;
	}
	for (i = 0; i < a->n; i++) {
		if (flag[i] == 0) {
			node = (struct array_node *)array_ref(a, i + 1);
			_free(node, ud);
		}
	}
}