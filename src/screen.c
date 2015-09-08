#include "screen.h"
#include "render.h"
#include "spritepack.h"

struct screen {
	int width;
	int height;
	float scale;
	float invw;
	float invh;
};

static struct screen SCREEN;

void screen_init(float w, float h, float scale) {
	SCREEN.width = (int)w;
	SCREEN.height = (int)h;
	SCREEN.scale = scale;
	SCREEN.invw = 2.0f / SCREEN_SCALE / w;
	SCREEN.invh = -2.0f / SCREEN_SCALE / h;
	render_setviewport(0, 0, (int)(w * scale), (int)(h * scale));
}

void screen_trans(float *x, float *y) {
	*x *= SCREEN.invw;
	*y *= SCREEN.invh;
}

void screen_scissor(int x, int y, int w, int h) {
	y = SCREEN.height - y - h;
	if (x < 0) {
		w += x;
		x = 0;
	} else if (x > SCREEN.width) {
		w = 0;
		h = 0;
	}
	if (y < 0) {
		h += y;
		y = 0;
	} else if (y > SCREEN.height) {
		w = 0;
		h = 0;
	}
	if (w <= 0 || h <= 0) {
		w = 0;
		h = 0;
	}
	x = (int)(x * SCREEN.scale);
	y = (int)(y * SCREEN.scale);
	w = (int)(w * SCREEN.scale);
	h = (int)(h * SCREEN.scale);

	render_setscissor(x, y, w, h);
}

int screen_visible(float x, float y) {
	return x >= 0.0f && x <= 2.0f && y >= -2.0f && y <= 0.0f;
}