#ifndef _FONT_H_
#define _FONT_H_

#ifdef __cplusplus
extern "C" {
#endif

	struct font_rect {
		int x;
		int y;
		int w;
		int h;
	};

	struct font;
	struct font *font_new(int width, int height);
	void font_free(struct font *f);
	const struct font_rect *font_lookup(struct font *f, int c, int font, int edge);
	const struct font_rect *font_insert(struct font *f, int c, int font, int width, int height, int edge);
	void font_remove(struct font *f, int c, int font, int edge);
	void font_flush(struct font *f);

#ifdef __cplusplus
};
#endif
#endif // _FONT_H_