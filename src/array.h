#ifndef _ARRAY_H_
#define _ARRAY_H_

#ifdef __cplusplus
extern "C" {
#endif

	struct block {
		char *data;
		int size;
	};
	void block_init(struct block *b, void *data, int size);
	void *block_slice(struct block *b, int size);

	struct array;
	struct array *array_new(struct block *b, int n, int size);
	int array_size(int n, int size);
	void *array_add(struct array *a);
	void array_rem(struct array *a, void *p);
	int array_id(struct array *a, void *p);
	void *array_ref(struct array *a, int id);
	void array_free(struct array *a, void(*free)(void *p, void *ud), void *ud);
#ifdef __cplusplus
};
#endif
#endif // _ARRAY_H_