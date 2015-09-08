#include "scissor.h"
#include "shader.h"
#include "screen.h"

#include <assert.h>

#define SCISSOR_MAX 8

struct box {
	int x;
	int y;
	int w;
	int h;
};

struct scissor {
	int depth;
	struct box b[SCISSOR_MAX];
};

static struct scissor S;

void scissor_push(int x, int y, int w, int h) {
	struct box *b;
	assert(S.depth < SCISSOR_MAX);
	shader_flush();
	if (S.depth == 0) {
		shader_scissor(1);
	}
	if (S.depth >= 1) {
		int newx, newy, bx, by, ax, ay;
		b = &S.b[S.depth - 1];
		newx = b->x > x ? b->x : x;
		newy = b->y > y ? b->y : y;
		bx = b->x + b->w;
		by = b->y + b->h;
		ax = x + w;
		ay = y + h;
		w = (bx > ax ? ax : bx) - newx;
		h = (by > ay ? ay : by) - newy;
		x = newx;
		y = newy;
	}
	b = &S.b[S.depth++];
	b->x = x;
	b->y = y;
	b->w = w;
	b->h = h;
	screen_scissor(x, y, w, h);
}
void scissor_pop(void) {
	struct box *b;
	assert(S.depth > 0);
	shader_flush();
	--S.depth;
	if (S.depth == 0) {
		shader_scissor(0);
		return;
	}
	b = &S.b[S.depth - 1];
	screen_scissor(b->x, b->y, b->w, b->h);
}