#ifndef _STREAM_H_
#define _STREAM_H_

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

	struct stream {
		char *data;
		int size;
	};
	void stream_init(struct stream *s, char *data, int size);
	int8_t stream_r8(struct stream *s);
	int16_t stream_r16(struct stream *s);
	int32_t stream_r32(struct stream *s);
	const char *stream_rstr(struct stream *s, int *size, void *(*slloc)(void *, int), void *ud);

	void stream_w8(struct stream *s, int8_t v);
	void stream_w16(struct stream *s, int16_t v);
	void stream_w32(struct stream *s, int32_t v);
	void stream_wstr(struct stream *s, char *v, int size);

	struct fstream {
		FILE *f;
	};
	void fstream_init(struct fstream *s, const char *file);
	void fstream_unit(struct fstream *s);
	int8_t fstream_r8(struct fstream *s);
	int16_t fstream_r16(struct fstream *s);
	int32_t fstream_r32(struct fstream *s);
	const char *fstream_rstr(struct fstream *s, int *size, void *(*slloc)(void *, int), void *ud);

	void fstream_w8(struct fstream *s, int8_t v);
	void fstream_w16(struct fstream *s, int16_t v);
	void fstream_w32(struct fstream *s, int32_t v);
	void fstream_wstr(struct fstream *s, char *v, int size);

#ifdef __cplusplus
};
#endif
#endif // _STREAM_H_