#include "label.h"
#include "array.h"
#include "readfile.h"
#include "pixel.h"

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

struct font_attr {
	int ascent;
	int descent;
	float scale;
};

struct font_global {
	stbtt_fontinfo info;
	char *data;
};

static struct font_global F;

void font_context_new(struct font_context *ctx, int font_size) {
	struct font_attr *font_attr = (struct font_attr *)malloc(sizeof *font_attr);
	font_attr->scale = stbtt_ScaleForPixelHeight(&F.info, font_size*1.0f);
	stbtt_GetFontVMetrics(&F.info, &(font_attr->ascent), &(font_attr->descent), 0);
	font_attr->ascent = (int)(font_attr->ascent * font_attr->scale);
	font_attr->descent = (int)(font_attr->descent * font_attr->scale);
	ctx->font = font_attr;
}

void font_context_free(struct font_context *ctx) {
	free(ctx->font);
	ctx->font = 0;
}

void font_context_size(struct font_context *ctx, const char *str, int unicode) {
	struct font_attr *font_attr;
	float s;
	int x0, y0, x1, y1;
	int adv;

	font_attr = (struct font_attr *)ctx->font;
	s = font_attr->scale;
	stbtt_GetCodepointBitmapBox(&F.info, unicode, s, s, &x0, &y0, &x1, &y1);
	stbtt_GetCodepointHMetrics(&F.info, unicode, &adv, 0);
	adv = (int)(adv * font_attr->scale);
	ctx->width = adv;
	ctx->height = font_attr->ascent + y1;
}

void font_context_glyph(struct font_context *ctx, const char *str, int unicode, void *buffer) {
	int x0, y0, x1, y1;
	int offset;
	struct font_attr *font_attr = (struct font_attr*)(ctx->font);
	float s = font_attr->scale;

	stbtt_GetCodepointBitmapBox(&F.info, unicode, s, s, &x0, &y0, &x1, &y1);
	offset = (font_attr->ascent + y0) * ctx->width + x0;
	if (offset >= 0) {
		unsigned char *p = (unsigned char *)buffer + offset;
		stbtt_MakeCodepointBitmap(&F.info, p, x1 - x0, y1 - y0, ctx->width, s, s, unicode);
	}
}

int font_context_init(const char *font) {
	int size;

	F.data = readfile(font, &size);
	if (!F.data) {
		pixel_log("font_context_init read:%s failed\n", font);
		return -1;
	}
	if (!stbtt_InitFont(&F.info, (unsigned char *)F.data, 0)) {
		pixel_log("stbtt_InitFont failed\n");
		return -1;
	}
	return 0;
}

void font_context_unit(void) {
	free(F.data);
}