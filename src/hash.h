#ifndef _HASH_H_
#define _HASH_H_

#ifdef __cplusplus
extern "C" {
#endif

	struct hash;
	struct hash *hash_new(long size);
	void hash_free(struct hash *h);
	long hash_insert(struct hash *h, const char *key, int size);
	long hash_exist(struct hash *h, const char *key, int size);
	void hash_remove(struct hash *h, long pos);

#ifdef __cplusplus
};
#endif
#endif // _HASH_H_
