#include "renderbuffer.h"
#include "screen.h"
#include "render.h"
#include "shader.h"
#include "texture.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>

void renderbuffer_init(struct renderbuffer *rb) {
	rb->bid = 0;
	rb->object = 0;
	rb->tid = 0;
}

void renderbuffer_unit(struct renderbuffer *rb) {
	if (rb->bid) {
		render_rem(VERTEXBUFFER, rb->bid);
		rb->bid = 0;
	}
}

void renderbuffer_update(struct renderbuffer *rb) {
	if (rb->bid == 0) {
		rb->bid = render_buffer_create(VERTEXBUFFER, rb->vb, rb->object * 4, sizeof(struct vertex));
	} else {
		render_buffer_update(rb->bid, rb->vb, rb->object * 4);
	}
}

void renderbuffer_clear(struct renderbuffer *rb) {
	rb->object = 0;
}

int renderbuffer_addvertex(struct renderbuffer *rb, const struct vertex_pack vp[4], uint32_t color, uint32_t addi) {
	struct quad *q;
	int i;

	if (rb->object >= MAX_COMMBINE) {
		return 1;
	}
	q = &rb->vb[rb->object];
	for (i = 0; i < 4; i++) {
		q->p[i].vp = vp[i];
		q->p[i].rgba[0] = (color >> 16) & 0xff;
		q->p[i].rgba[1] = (color >> 8) & 0xff;
		q->p[i].rgba[2] = (color)& 0xff;
		q->p[i].rgba[3] = (color >> 24) & 0xff;
		q->p[i].addi[0] = (addi >> 16) & 0xff;
		q->p[i].addi[1] = (addi >> 8) & 0xff;
		q->p[i].addi[2] = (addi)& 0xff;
		q->p[i].addi[3] = (addi >> 24) & 0xff;
	}
	if (++rb->object >= MAX_COMMBINE) {
		return 1;
	}
	return 0;
}

void renderbuffer_draw(struct renderbuffer *rb, float x, float y, float scale) {
	shader_drawbuffer(rb, x*SCREEN_SCALE, y*SCREEN_SCALE, scale);
}


#ifdef PIXEL_LUA

#include "lua.h"
#include "lauxlib.h"

static int ladd(lua_State *L) {
	struct renderbuffer *rb = (struct renderbuffer *)lua_touserdata(L, 1);
	if (lua_isuserdata(L, 2)) {
		struct sprite *s = (struct sprite *)lua_touserdata(L, 2);
		renderbuffer_add(rb, s);
	} else {
		int n, np;
		uint32_t additive;
		uint32_t color = 0xffffffff;
		int tid = (int)luaL_checkinteger(L, 2);
		int rid = texture_rid(tid);
		if (rid <= 0) {
			lua_pushboolean(L, 0);
			return 1;
		}
		rb->tid = tid;
		luaL_checktype(L, 3, LUA_TTABLE);
		luaL_checktype(L, 4, LUA_TTABLE);
		if (!lua_isnoneornil(L, 5)) {
			color = (uint32_t)luaL_checkinteger(L, 5);
		}
		additive = (uint32_t)luaL_optinteger(L, 6, 0);

		n = lua_rawlen(L, 3);
		np = n / 2;
		if (lua_rawlen(L, 4) != n || n % 2 != 0) {
			return luaL_error(L, "rbuffer.add invalid texture or screen");
		} else {
			int i;
#if defined(_MSC_VER)
			struct vertex_pack *vp = (struct vertex_pack *)_alloca(np * sizeof(struct vertex_pack));
#else
			struct vertex_pack vp[np];
#endif
			for (i = 0; i < np; i++) {
				float tx, ty;
				lua_rawgeti(L, 3, i * 2 + 1);
				lua_rawgeti(L, 3, i * 2 + 2);
				tx = (float)lua_tonumber(L, -2);
				ty = (float)lua_tonumber(L, -1);
				lua_pop(L, 2);
				texture_coord(tid, tx, ty, &vp[i].tx, &vp[i].ty);
			}
			for (i = 0; i < np; i++) {
				lua_rawgeti(L, 4, i * 2 + 1);
				lua_rawgeti(L, 4, i * 2 + 2);
				vp[i].vx = (float)lua_tonumber(L, -2);
				vp[i].vy = (float)lua_tonumber(L, -1);
				lua_pop(L, 2);
			}
			renderbuffer_addvertex(rb, vp, color, additive);
		}
	}
	return 0;
}

static int lupdate(lua_State *L) {
	struct renderbuffer *rb = (struct renderbuffer *)lua_touserdata(L, 1);
	renderbuffer_update(rb);
	return 0;
}

static int ldraw(lua_State *L) {
	struct renderbuffer *rb = (struct renderbuffer *)lua_touserdata(L, 1);
	float x = (float)luaL_optnumber(L, 2, 0.0f);
	float y = (float)luaL_optnumber(L, 3, 0.0f);
	float scale = (float)luaL_optnumber(L, 4, 1.0f);
	renderbuffer_draw(rb, x, y, scale);
	return 0;
}

static int lfree(lua_State *L) {
	struct renderbuffer *rb = (struct renderbuffer *)lua_touserdata(L, 1);
	renderbuffer_unit(rb);
	return 0;
}

static int lnew(lua_State *L) {
	struct renderbuffer *rb = (struct renderbuffer *)lua_newuserdata(L, sizeof *rb);
	renderbuffer_init(rb);
	if (luaL_newmetatable(L, "renderbuffer")) {
		luaL_Reg l[] = {
			{"add", ladd},
			{"update", lupdate},
			{"draw", ldraw},
			{0, 0},
		};
		luaL_newlib(L, l);
		lua_setfield(L, -2, "__index");
		lua_pushcfunction(L, lfree);
		lua_setfield(L, -2, "__gc");
	}
	lua_setmetatable(L, -2);
	return 1;
}

int pixel_renderbuffer(lua_State *L) {
	luaL_Reg l[] = {
		{"new", lnew},
		{0, 0},
	};
	luaL_newlib(L, l);
	return 1;
}

#endif // PIXEL_LUA