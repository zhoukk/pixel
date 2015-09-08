#ifndef _LABEL_H_
#define _LABEL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "spritepack.h"
#include "matrix.h"
#include <stdio.h>

#define LABEL_ALIGN_LEFT 0
#define LABEL_ALIGN_RIGHT 1
#define LABEL_ALIGN_CENTER 2

#define RL_COLOR 1
#define RL_LINEFEED 2

	struct label_field {
		struct {
			uint32_t type : 8;
			uint32_t start : 12;
			uint32_t end : 12;
		};
		union {
			uint32_t color;
			int val;
		};
	};

	struct rich_text {
		int count;
		int width;
		int height;
		const char *text;
		struct label_field *fields;
	};

	struct font_context {
		int width;
		int height;
		int ascent;
		void *font;
		void *dc;
	};

	void label_init(const char *font);
	void label_unit(void);
	void label_flush(void);
	void label_draw(const struct rich_text *rich, struct pack_label *pl, struct srt *srt, const struct sprite_trans *trans);
	void label_size(const char *str, struct pack_label *pl, const char *chr, int *width, int *height);
	int label_char_size(struct pack_label *pl, const char *chr, int *width, int *height, int *unicode);
	uint32_t label_color(struct pack_label *pl, const struct sprite_trans *trans);

	int font_context_init(const char *font);
	void font_context_unit(void);
	void font_context_size(struct font_context *ctx, const char *str, int unicode);
	void font_context_glyph(struct font_context *ctx, const char *str, int unicode, void *buffer);
	void font_context_new(struct font_context *ctx, int font_size);
	void font_context_free(struct font_context *ctx);

#ifdef __cplusplus
};
#endif
#endif // _LABEL_H_