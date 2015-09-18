#include "screen.h"
#include "renderbuffer.h"
#include "shader.h"
#include "pixel.h"

#include "lua.h"
#include "lauxlib.h"

#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <malloc.h>

static int geometry_program = 0;

static uint32_t convert_color(uint32_t c) {
	uint32_t red, green, blue;
	uint32_t alpha = (c >> 24) & 0xff;
	if (alpha == 0xff) {
		return c;
	}
	if (alpha == 0) {
		return c | 0xff000000;
	}
	red = (c >> 16) & 0xff;
	green = (c >> 8) & 0xff;
	blue = (c)& 0xff;
	red = red * alpha / 255;
	green = green * alpha / 255;
	blue = blue * alpha / 255;
	return alpha << 24 | red << 16 | green << 8 | blue;
}

static int lline(lua_State *L) {
	float x1 = (float)(luaL_checknumber(L, 1) * SCREEN_SCALE);
	float y1 = (float)(luaL_checknumber(L, 2) * SCREEN_SCALE);
	float x2 = (float)(luaL_checknumber(L, 3) * SCREEN_SCALE);
	float y2 = (float)(luaL_checknumber(L, 4) * SCREEN_SCALE);
	uint32_t color = convert_color((uint32_t)luaL_checkinteger(L, 5));
	struct vertex_pack vp[4];
	int i;

	vp[0].vx = x1;
	vp[0].vy = y1;
	vp[1].vx = x2;
	vp[1].vy = y2;
	if (fabs(x1 - x2) > fabs(y1 - y2)) {
		vp[2].vx = x2;
		vp[2].vy = y2 + SCREEN_SCALE;
		vp[3].vx = x1;
		vp[3].vy = y1 + SCREEN_SCALE;
	} else {
		vp[2].vx = x2 + SCREEN_SCALE;
		vp[2].vy = y2;
		vp[3].vx = x1 + SCREEN_SCALE;
		vp[3].vy = y1;
	}
	for (i = 0; i < 4; i++) {
		vp[i].tx = 0;
		vp[i].ty = 0;
		screen_trans(&vp[i].vx, &vp[i].vy);
	}
	shader_program(geometry_program, 0);
	shader_drawvertex(vp, color, 0);
	return 0;
}

static int lbox(lua_State *L) {
	float x = (float)(luaL_checknumber(L, 1) * SCREEN_SCALE);
	float y = (float)(luaL_checknumber(L, 2) * SCREEN_SCALE);
	float w = (float)(luaL_checknumber(L, 3) * SCREEN_SCALE);
	float h = (float)(luaL_checknumber(L, 4) * SCREEN_SCALE);
	uint32_t color = convert_color((uint32_t)luaL_checkinteger(L, 5));
	struct vertex_pack vp[4];
	int i;

	vp[0].vx = x;
	vp[0].vy = y;
	vp[1].vx = x + w;
	vp[1].vy = y;
	vp[2].vx = x + w;
	vp[2].vy = y + h;
	vp[3].vx = x;
	vp[3].vy = y + h;
	for (i = 0; i < 4; i++) {
		vp[i].tx = 0;
		vp[i].ty = 0;
		screen_trans(&vp[i].vx, &vp[i].vy);
	}
	shader_program(geometry_program, 0);
	shader_drawvertex(vp, color, 0);
	return 0;
}

static int lpolygon(lua_State *L) {
	int n = lua_rawlen(L, 1);
	uint32_t color = convert_color((uint32_t)luaL_checkinteger(L, 2));
	int np = n / 2;
	int i;
#ifdef _MSC_VER
	struct vertex_pack *vp = (struct vertex_pack *)_alloca(np*sizeof(struct vertex_pack));
#else
	struct vertex_pack vp[np];
#endif
	if (np * 2 != n) {
		return luaL_error(L, "invalid polygon");
	}
	for (i = 0; i < np; i++) {
		float vx, vy;
		lua_rawgeti(L, 1, i * 2 + 1);
		lua_rawgeti(L, 1, i * 2 + 2);
		vx = (float)(lua_tonumber(L, -2) * SCREEN_SCALE);
		vy = (float)(lua_tonumber(L, -1) * SCREEN_SCALE);
		lua_pop(L, 2);
		screen_trans(&vx, &vy);
		vp[i].vx = vx;
		vp[i].vy = vy;
		vp[i].tx = 0;
		vp[i].ty = 0;
	}
	shader_program(geometry_program, 0);
	if (np == 4) {
		shader_drawvertex(vp, color, 0);
	} else {
		shader_drawpolygon(np, vp, color, 0);
	}
	return 0;
}

static int lprogram(lua_State *L) {
	geometry_program = (int)luaL_checkinteger(L, 1);
	return 0;
}

int pixel_geometry(lua_State *L) {
	luaL_Reg l[] = {
		{"line", lline},
		{"box", lbox},
		{"polygon", lpolygon},
		{"program", lprogram},
		{0, 0},
	};
	luaL_newlib(L, l);
	return 1;
}

