#include "label.h"
#include "font.h"
#include "color.h"
#include "renderbuffer.h"
#include "render.h"
#include "shader.h"
#include "screen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>

#define TEX_HEIGHT 1024
#define TEX_WIDTH 1024
#define FONT_SIZE 31
#define TEX_FMT TEXTURE_A8

static int Tid = -1;
static struct font *F = 0;

void label_init(const char *font) {
	if (F || !font) return;
	if (font_context_init(font) != 0) return;
	F = font_new(TEX_WIDTH, TEX_HEIGHT);
	Tid = render_texture_create(TEX_WIDTH, TEX_HEIGHT, TEX_FMT, TEXTURE_2D, 0);
	render_texture_update(Tid, TEX_WIDTH, TEX_HEIGHT, 0, 0, 0);
}

void label_unit(void) {
	if (F) {
		render_rem(TEXTURE, Tid);
		font_free(F);
		font_context_unit();
	}
}

void label_flush(void) {
	if (F) {
		font_flush(F);
	}
}

static inline int copystr(char *utf8, const char *str, int n) {
	int i;
	int unicode;

	utf8[0] = str[0];
	unicode = utf8[0] & ((1 << (8 - n)) - 1);
	for (i = 1; i < n; i++) {
		utf8[i] = str[i];
		unicode = unicode << 6 | (utf8[i] & 0x3f);
	}
	utf8[i] = 0;
	return unicode;
}

static inline int get_unicode(const char *str, int n) {
	int i;
	int unicode = str[0] & ((1 << (8 - n)) - 1);
	for (i = 1; i < n; i++) {
		unicode = unicode << 6 | ((uint8_t)str[i] & 0x3f);
	}
	return unicode;
}

static inline int max4(int a, int b, int c, int d) {
	a = a > b ? a : b;
	a = a > c ? a : c;
	a = a > d ? a : d;
	return a;
}

static void gen_outline(int w, int h, uint8_t *buffer, uint8_t *dest) {
	int i, j;
	for (i = 0; i < h; i++) {
		uint8_t * output = dest + i*w;
		uint8_t * line = buffer + i*w;
		uint8_t * prev;
		uint8_t * next;
		if (i == 0) {
			prev = line;
		} else {
			prev = line - w;
		}
		if (i == h - 1) {
			next = line;
		} else {
			next = line + w;
		}
		for (j = 0; j < w; j++) {
			int left, right;
			int n1, n2, edge;
			if (j == 0) {
				left = 0;
			} else {
				left = 1;
			}
			if (j == w - 1) {
				right = 0;
			} else {
				right = 1;
			}
			n1 = max4(line[j - left], line[j + right], prev[j], next[j]);
			n2 = max4(prev[j - left], prev[j + right], next[j - left], next[j + right]);
			edge = (n1 * 3 + n2) / 4;
			if (line[j] == 0) {
				output[j] = edge / 2;
			} else {
				output[j] = line[j] / 2 + 128;
			}
		}
		if (output[0] > 128) {
			output[0] /= 2;
		}
		if (output[w - 1] > 128) {
			output[w - 1] /= 2;
		}
	}
}

/*
static void write_pgm(int unicode, int w, int h, const uint8_t *buffer) {
	char tmp[128];
	FILE *f;
	int i,j;

	sprintf(tmp,"%d.pgm",unicode);
	f = fopen(tmp, "wb");
	fprintf(f, "P2\n%d %d\n255\n",w,h);
	for (i=0;i<h;i++) {
		for (j=0;j<w;j++) {
			fprintf(f,"%3d,",buffer[0]);
			++buffer;
		}
		fprintf(f,"\n");
	}
	fclose(f);
}
*/

static const struct font_rect *gen_char(int unicode, const char *utf8, int size, int edge) {
	// todo : use large size when size is large
	const struct font_rect *rect;
	struct font_context ctx;
#if defined(_MSC_VER)
	char *buffer, *tmp;
#endif
	int buffer_sz;
	font_context_new(&ctx, FONT_SIZE);
	font_context_size(&ctx, utf8, unicode);
	rect = font_insert(F, unicode, FONT_SIZE, ctx.width + 1, ctx.height + 1, edge);
	if (rect == 0) {
		font_flush(F);
		rect = font_insert(F, unicode, FONT_SIZE, ctx.width + 1, ctx.height + 1, edge);
		if (rect == 0) {
			font_context_free(&ctx);
			return 0;
		}
	}
	ctx.width = rect->w;
	ctx.height = rect->h;
	buffer_sz = ctx.width * ctx.height;
#if defined(_MSC_VER)
	buffer = _alloca(buffer_sz);
	tmp = _alloca(buffer_sz);
#else
	char buffer[buffer_sz];
	char tmp[buffer_sz];
#endif
	memset(tmp, 0, buffer_sz);
	font_context_glyph(&ctx, utf8, unicode, tmp);
	gen_outline(ctx.width, ctx.height, (uint8_t *)tmp, (uint8_t *)buffer);

	font_context_free(&ctx);
	// write_pgm(unicode, ctx.width, ctx.height, (const uint8_t *)buffer);
	render_texture_subupdate(Tid, buffer, rect->x, rect->y, rect->w, rect->h);
	return rect;
}


static inline void set_point(struct vertex_pack *v, int *m, int xx, int yy, int tx, int ty) {
	v->vx = (xx * m[0] + yy * m[2]) / 1024.0f + m[4];
	v->vy = (xx * m[1] + yy * m[3]) / 1024.0f + m[5];
	screen_trans(&v->vx, &v->vy);

	v->tx = (uint16_t)(tx * (65535.0f / TEX_WIDTH));
	v->ty = (uint16_t)(ty * (65535.0f / TEX_HEIGHT));
}

static void draw_rect(const struct font_rect *rect, int size, struct matrix *mat, uint32_t color, uint32_t addi) {
	struct vertex_pack vb[4];

	int w = (rect->w - 1) * size / FONT_SIZE;
	int h = (rect->h - 1) * size / FONT_SIZE;

	set_point(&vb[0], mat->m, 0, 0, rect->x, rect->y);
	set_point(&vb[1], mat->m, w*SCREEN_SCALE, 0, rect->x + rect->w - 1, rect->y);
	set_point(&vb[2], mat->m, w*SCREEN_SCALE, h*SCREEN_SCALE, rect->x + rect->w - 1, rect->y + rect->h - 1);
	set_point(&vb[3], mat->m, 0, h*SCREEN_SCALE, rect->x, rect->y + rect->h - 1);
	shader_drawvertex(vb, color, addi);
}

static int draw_size(int unicode, const char *utf8, int size, int edge) {
	const struct font_rect *rect = font_lookup(F, unicode, FONT_SIZE, edge);
	if (rect == 0) {
		rect = gen_char(unicode, utf8, size, edge);
	}
	if (rect) {
		return (rect->w - 1) * size / FONT_SIZE;
	}
	return 0;
}

static int draw_height(int unicode, const char *utf8, int size, int edge) {
	const struct font_rect *rect = font_lookup(F, unicode, FONT_SIZE, edge);
	if (rect == 0) {
		rect = gen_char(unicode, utf8, size, edge);
	}
	if (rect) {
		return rect->h * size / FONT_SIZE;
	}
	return 0;
}

static struct font_context char_size(int unicode, const char *utf8, int size, int edge) {
	struct font_context ctx;
	const struct font_rect * rect = font_lookup(F, unicode, FONT_SIZE, edge);
	font_context_new(&ctx, FONT_SIZE);
	if (ctx.font == 0) {
		ctx.width = 0;
		ctx.height = 0;
		return ctx;
	}
	if (rect == 0) {
		font_context_size(&ctx, utf8, unicode);
		//see gen_char
		ctx.width += 1;
		ctx.height += 1;
		ctx.width = (ctx.width - 1) * size / FONT_SIZE;
		ctx.height = ctx.height * size / FONT_SIZE;
	} else {
		ctx.width = (rect->w - 1) * size / FONT_SIZE;
		ctx.height = rect->h * size / FONT_SIZE;
	}
	font_context_free(&ctx);
	return ctx;
}

static int draw_utf8(int unicode, float cx, int cy, int size, const struct srt *srt, uint32_t color, const struct sprite_trans *arg, int edge) {
	struct matrix tmp;
	struct matrix mat1 = { { 1024, 0, 0, 1024, (int)(cx*SCREEN_SCALE), (int)(cy*SCREEN_SCALE) } };
	struct matrix *m;
	const struct font_rect *rect = font_lookup(F, unicode, FONT_SIZE, edge);
	if (rect == 0) {
		return 0;
	}
	if (arg->mat) {
		m = &tmp;
		matrix_mul(m, &mat1, arg->mat);
	} else {
		m = &mat1;
	}
	matrix_srt(m, srt);
	draw_rect(rect, size, m, color, arg->addi);
	return (rect->w - 1) * size / FONT_SIZE;
}

static uint32_t get_rich_field_color(const struct rich_text *rich, int idx) {
	int i;
	uint32_t pos = (uint32_t)idx;
	for (i = 0; i < rich->count; i++) {
		struct label_field *field = (struct label_field *)(rich->fields + i);
		if (pos >= field->start && pos <= field->end && field->type == RL_COLOR) {
			return field->color;
		}
	}
	return 0;
}

static int get_rich_field_lf(const struct rich_text *rich, int idx, float * offset) {
	int i;
	for (i = 0; i < rich->count; i++) {
		struct label_field *field = (struct label_field*)(rich->fields + i);
		if (idx == field->start && idx == field->end && field->type == RL_LINEFEED) {
			*offset = (float)(field->val / 1000.0);
			return 1;
		}
	}
	return 0;
}

static int unicode_len(const char chr) {
	uint8_t c = (uint8_t)chr;
	if ((c & 0x80) == 0) {
		return 1;
	} else if ((c & 0xe0) == 0xc0) {
		return 2;
	} else if ((c & 0xf0) == 0xe0) {
		return 3;
	} else if ((c & 0xf8) == 0xf0) {
		return 4;
	} else if ((c & 0xfc) == 0xf8) {
		return 5;
	} else {
		return 6;
	}
}

static void draw_line(const struct rich_text *rich, struct pack_label * l, struct srt *srt, const struct sprite_trans *arg, uint32_t color, int cy, int w, int start, int end, int *pre_char_cnt, float space_scale) {
	const char *str = rich->text;
	float cx;
	int j;
	int size = l->size;
	int char_cnt = 0;
	if (l->auto_scale != 0 && w > l->width) {
		float scale = l->width * 1.0f / w;
		size = (int)(scale * size);
		cy = cy + (l->size - size) / 2;
		w = l->width;
	}
	switch (l->align) {
	case LABEL_ALIGN_LEFT:
		cx = 0.0f;
		break;
	case LABEL_ALIGN_RIGHT:
		cx = (l->width - w)*1.0f;
		break;
	case LABEL_ALIGN_CENTER:
		cx = (l->width - w) / 2.0f;
		break;
	default:
		cx = 0.0f;
		break;
	}
	for (j = start; j < end;) {
		int unicode;
		int len;
		char_cnt++;
		len = unicode_len(str[j]);
		unicode = get_unicode(str + j, len);
		j += len;
		if (unicode != '\n') {
			uint32_t field_color = get_rich_field_color(rich, *pre_char_cnt + char_cnt);
			if (field_color == 0) {
				field_color = color;
			} else {
				field_color = color_mul(field_color, color | 0xffffff);
			}
			cx += (draw_utf8(unicode, cx, cy, size, srt, field_color, arg, l->edge) + l->space_w)*space_scale;
		}
	}
	*pre_char_cnt += char_cnt;
}


void label_draw(const struct rich_text *rich, struct pack_label *pl, struct srt *srt, const struct sprite_trans *trans) {
	uint32_t color;
	const char *str;
	char utf8[7];
	int i;
	int ch = 0, w = 0, cy = 0, pre = 0, char_cnt = 0, idx = 0;

	shader_texture(Tid, 0);
	color = label_color(pl, trans);
	str = rich->text;

	for (i = 0; str&&str[i];) {
		float space_scale = 1.0;
		uint32_t lf;
		int len = unicode_len(str[i]);
		int unicode = copystr(utf8, str + i, len);
		i += len;
		w += draw_size(unicode, utf8, pl->size, pl->edge) + pl->space_w;
		if (ch == 0) {
			ch = draw_height(unicode, utf8, pl->size, pl->edge) + pl->space_h;
		}
		lf = get_rich_field_lf(rich, idx, &space_scale);
		if ((pl->auto_scale == 0 && lf) || unicode == '\n') {
			draw_line(rich, pl, srt, trans, color, cy, w, pre, i, &char_cnt, space_scale);
			cy += ch;
			pre = i;
			w = 0;
			ch = 0;
		}
		idx++;
	}
	draw_line(rich, pl, srt, trans, color, cy, w, pre, i, &char_cnt, 1.0f);
}

void label_size(const char *str, struct pack_label *pl, const char *chr, int *width, int *height) {
	char utf8[7];
	int i, w = 0, h = 0, maxw = 0, maxh = 0;
	for (i = 0; str[i];) {
		struct font_context ctx;
		int unicode;
		int len = unicode_len(str[i]);
		unicode = copystr(utf8, str + i, len);
		i += len;
		ctx = char_size(unicode, utf8, pl->size, pl->edge);
		w += ctx.width + pl->space_w;
		if (h == 0) {
			h = ctx.height + pl->space_h;
		}
		if ((pl->auto_scale == 0 && w > pl->width) || unicode == '\n') {
			maxh += h;
			h = 0;
			if (w > maxw) {
				maxw = w;
			} else {
				w = 0;
			}
		}
	}
	maxh += h;
	if (w > maxw) {
		maxw = w;
	}
	if (pl->auto_scale > 0 && maxw > pl->width) {
		maxw = pl->width;
	}
	*width = maxw;
	*height = maxh;
}

int label_char_size(struct pack_label *pl, const char *chr, int *width, int *height, int *unicode) {
	char utf8[7];
	struct font_context ctx;
	int len = unicode_len(chr[0]);
	*unicode = copystr(utf8, chr, len);
	ctx = char_size(*unicode, utf8, pl->size, pl->edge);
	*width = ctx.width + pl->space_w;
	*height = ctx.height + pl->space_h;
	return len;
}

uint32_t label_color(struct pack_label *pl, const struct sprite_trans *trans) {
	uint32_t color;
	if (trans->color == 0xffffffff) {
		color = pl->color;
	} else if (pl->color == 0xffffffff) {
		color = trans->color;
	} else {
		color = color_mul(pl->color, trans->color);
	}
	return color;
}
