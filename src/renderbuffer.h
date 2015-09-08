#ifndef _RENDERBUFFER_H_
#define _RENDERBUFFER_H_

#include <stdint.h>

#define MAX_COMMBINE 1024

#ifdef __cplusplus
extern "C" {
#endif

	struct vertex_pack {
		float vx;
		float vy;
		uint16_t tx;
		uint16_t ty;
	};

	struct vertex {
		struct vertex_pack vp;
		uint8_t rgba[4];
		uint8_t addi[4];
	};

	struct quad {
		struct vertex p[4];
	};

	struct renderbuffer {
		int object;
		int tid;
		int bid;
		struct quad vb[MAX_COMMBINE];
	};

	struct sprite;
	void renderbuffer_init(struct renderbuffer *rb);
	void renderbuffer_unit(struct renderbuffer *rb);
	void renderbuffer_update(struct renderbuffer *rb);
	int renderbuffer_addvertex(struct renderbuffer *rb, const struct vertex_pack vp[4], uint32_t color, uint32_t addi);
	void renderbuffer_clear(struct renderbuffer *rb);
	void renderbuffer_draw(struct renderbuffer *rb, float x, float y, float scale);
	int renderbuffer_add(struct renderbuffer *rb, struct sprite *s);

#ifdef PIXEL_LUA
#include "lua.h"
	int pixel_renderbuffer(lua_State *L);
#endif // PIXEL_LUA

#ifdef __cplusplus
};
#endif
#endif // _RENDERBUFFER_H_
