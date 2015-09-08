#include "stream.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void stream_init(struct stream *s, char *data, int size) {
	s->data = data;
	s->size = size;
}

int8_t stream_r8(struct stream *s) {
	int8_t c;
	assert(s->size > 0);
	c = (int8_t)*(s->data);
	++s->data;
	--s->size;
	return c;
}

int16_t stream_r16(struct stream *s) {
	uint8_t low, high;

	assert(s->size >= 2);
	low = (uint8_t)*(s->data);
	high = (uint8_t)*(s->data + 1);
	s->data += 2;
	s->size -= 2;
	return (int16_t)(low | (uint16_t)high << 8);
}

int32_t stream_r32(struct stream *s) {
	uint8_t b[4];

	assert(s->size >= 4);
	b[0] = (uint8_t)*(s->data);
	b[1] = (uint8_t)*(s->data + 1);
	b[2] = (uint8_t)*(s->data + 2);
	b[3] = (uint8_t)*(s->data + 3);
	s->data += 4;
	s->size -= 4;
	return (int32_t)(b[0] | (uint32_t)b[1] << 8 | (uint32_t)b[2] << 16 | (uint32_t)b[3] << 24);
}

const char *stream_rstr(struct stream *s, int *size, void *(*slloc)(void *, int), void *ud) {
	uint8_t n;
	char *str;

	n = (uint8_t)stream_r8(s);
	if (n == 255) {
		return 0;
	}
	assert(s->size >= n);
	str = slloc(ud, (n + 1 + 3)&~3);
	memcpy(str, s->data, n);
	str[n] = 0;
	s->data += n;
	s->size -= n;
	if (size) {
		*size = n;
	}
	return str;
}

void stream_w8(struct stream *s, int8_t v) {
	uint8_t b[1] = { (int8_t)v };
	assert(s->size > 0);
	memcpy(s->data, b, 1);
	++s->data;
	--s->size;
}

void stream_w16(struct stream *s, int16_t v) {
	uint8_t b[2] = { (uint8_t)v & 0xff,(uint8_t)((v >> 8) & 0xff) };
	assert(s->size >= 2);
	memcpy(s->data, b, 2);
	s->data += 2;
	s->size -= 2;
}

void stream_w32(struct stream *s, int32_t v) {
	uint8_t b[4] = {
		(uint8_t)v & 0xff,
		(uint8_t)((v >> 8) & 0xff),
		(uint8_t)((v >> 16) & 0xff),
		(uint8_t)((v >> 24) & 0xff),
	};
	assert(s->size >= 4);
	memcpy(s->data, b, 4);
	s->data += 4;
	s->size -= 4;
}

void stream_wstr(struct stream *s, char *v, int size) {
	assert(s->size > size);
	stream_w8(s, size);
	memcpy(s->data, v, size);
	s->data += size;
	s->size -= size;
}

void fstream_init(struct fstream *s, const char *file) {
	s->f = fopen(file, "wb");
}

void fstream_unit(struct fstream *s) {
	fclose(s->f);
}

int8_t fstream_r8(struct fstream *s) {
	int8_t c;
	assert(s->f);
	fread(&c, 1, 1, s->f);
	return c;
}

int16_t fstream_r16(struct fstream *s) {
	uint8_t low, high;

	assert(s->f);
	fread(&low, 1, 1, s->f);
	fread(&high, 1, 1, s->f);
	return (int16_t)(low | (uint16_t)high << 8);
}

int32_t fstream_r32(struct fstream *s) {
	uint8_t b[4];

	assert(s->f);
	fread(b, 1, 1, s->f);
	fread(b + 1, 1, 1, s->f);
	fread(b + 2, 1, 1, s->f);
	fread(b + 3, 1, 1, s->f);
	return (int32_t)(b[0] | (uint32_t)b[1] << 8 | (uint32_t)b[2] << 16 | (uint32_t)b[3] << 24);
}

const char *fstream_rstr(struct fstream *s, int *size, void *(*slloc)(void *, int), void *ud) {
	uint8_t n;
	char *str;

	n = (uint8_t)fstream_r8(s);
	if (n == 255) {
		return 0;
	}
	assert(s->f);
	str = slloc(ud, (n + 1 + 3)&~3);
	fread(str, n, 1, s->f);
	str[n] = 0;
	if (size) {
		*size = n;
	}
	return str;
}

void fstream_w8(struct fstream *s, int8_t v) {
	uint8_t b[1] = { (int8_t)v };
	assert(s->f);
	fwrite(b, 1, 1, s->f);
}

void fstream_w16(struct fstream *s, int16_t v) {
	uint8_t b[2] = { (uint8_t)v & 0xff,(uint8_t)((v >> 8) & 0xff) };
	assert(s->f);
	fwrite(b, 2, 1, s->f);
}

void fstream_w32(struct fstream *s, int32_t v) {
	uint8_t b[4] = {
		(uint8_t)v & 0xff,
		(uint8_t)((v >> 8) & 0xff),
		(uint8_t)((v >> 16) & 0xff),
		(uint8_t)((v >> 24) & 0xff),
	};
	assert(s->f);
	fwrite(b, 4, 1, s->f);
}

void fstream_wstr(struct fstream *s, char *v, int size) {
	assert(s->f);
	fstream_w8(s, size);
	fwrite(v, size, 1, s->f);
}