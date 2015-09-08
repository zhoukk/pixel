#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned long _crypt_table[0x500];
static char _ctypt_init = 0;
static const unsigned long HASH_OFF = 0, HASH_A = 1, HASH_B = 2;

#pragma pack(1)
struct hash_node {
	long hasha;
	long hashb;
	char exist;
};
#pragma pack()

struct hash {
	unsigned long size;
	struct hash_node *node;
};

static void _crypt_table_init(void) {
	unsigned long seed = 0x00100001, idx1 = 0, idx2 = 0, i;
	for (idx1 = 0; idx1 < 0x100; idx1++) {
		for (idx2 = idx1, i = 0; i < 5; i++, idx2 += 0x100) {
			unsigned long tmp1, tmp2;
			seed = (seed * 125 + 3) % 0x2AAAAB;
			tmp1 = (seed & 0xFFFF) << 0x10;
			seed = (seed * 125 + 3) % 0x2AAAAB;
			tmp2 = (seed & 0xFFFF);
			_crypt_table[idx2] = (tmp1 | tmp2);
		}
	}
}

static unsigned long _hash_string(const char *key, size_t size, unsigned long type) {
	unsigned long seed1 = 0x7FED7FED, seed2 = 0xEEEEEEEE;
	size_t i = 0;
	while (i < size) {
		int ch = toupper(*(key + i++));
		seed1 = _crypt_table[(type << 8) + ch] ^ (seed1 + seed2);
		seed2 = ch + seed1 + seed2 + (seed2 << 5) + 3;
	}
	return seed1;
}

struct hash *hash_new(long size) {
	struct hash *h;
	long i;
	h = malloc(sizeof(struct hash));
	if (!h) return 0;
	if (size == 0) {
		size = 16;
	}
	h->size = size;
	h->node = malloc(size * sizeof(struct hash_node));
	if (!h->node) {
		free(h);
		return 0;
	}
	for (i = 0; i < size; i++) {
		h->node[i].hasha = -1;
		h->node[i].hashb = -1;
		h->node[i].exist = 0;
	}
	if (_ctypt_init == 0) {
		_crypt_table_init();
		_ctypt_init = 1;
	}
	return h;
}

void hash_free(struct hash *h) {
	free(h->node);
	free(h);
}

long hash_insert(struct hash *h, const char *key, size_t size) {
	unsigned long hash = _hash_string(key, size, HASH_OFF);
	unsigned long hasha = _hash_string(key, size, HASH_A);
	unsigned long hashb = _hash_string(key, size, HASH_B);
	unsigned long hash_start = hash % h->size;
	unsigned long npos = hash_start;
	while (h->node[npos].exist == 1) {
		if (h->node[npos].hasha == hasha && h->node[npos].hashb == hashb) {
			return -1;
		}
		npos = (npos + 1) % h->size;
		if (npos == hash_start) {
			return -1;
		}
	}
	h->node[npos].exist = 1;
	h->node[npos].hasha = hasha;
	h->node[npos].hashb = hashb;
	return npos;
}

long hash_exist(struct hash *h, const char *key, size_t size) {
	unsigned long hash = _hash_string(key, size, HASH_OFF);
	unsigned long hasha = _hash_string(key, size, HASH_A);
	unsigned long hashb = _hash_string(key, size, HASH_B);
	unsigned long hash_start = hash % h->size;
	unsigned long npos = hash_start;
	while (1) {
		if (h->node[npos].hasha == hasha && h->node[npos].hashb == hashb) {
			return npos;
		} else {
			npos = (npos + 1) % h->size;
		}
		if (npos == hash_start) {
			break;
		}
	}
	return -1;
}

void hash_remove(struct hash *h, long pos) {
	h->node[pos].exist = 0;
	h->node[pos].hasha = -1;
	h->node[pos].hashb = -1;
}
