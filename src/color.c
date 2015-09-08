#include "color.h"

#include <stdio.h>
#include <stdlib.h>

static inline unsigned int clamp(unsigned int c) {
	return ((c) > 255 ? 255 : (c));
}

uint32_t color_mul(uint32_t c1, uint32_t c2) {
	int r1 = (c1 >> 24) & 0xff;
	int g1 = (c1 >> 16) & 0xff;
	int b1 = (c1 >> 8) & 0xff;
	int a1 = (c1)& 0xff;
	int r2 = (c2 >> 24) & 0xff;
	int g2 = (c2 >> 16) & 0xff;
	int b2 = (c2 >> 8) & 0xff;
	int a2 = c2 & 0xff;

	return (r1 * r2 / 255) << 24 |
		(g1 * g2 / 255) << 16 |
		(b1 * b2 / 255) << 8 |
		(a1 * a2 / 255);
}

uint32_t color_add(uint32_t c1, uint32_t c2) {
	int r1 = (c1 >> 16) & 0xff;
	int g1 = (c1 >> 8) & 0xff;
	int b1 = (c1)& 0xff;
	int r2 = (c2 >> 16) & 0xff;
	int g2 = (c2 >> 8) & 0xff;
	int b2 = (c2)& 0xff;
	return clamp(r1 + r2) << 16 | clamp(g1 + g2) << 8 | clamp(b1 + b2);
}

#ifdef PIXEL_LUA

#include "lua.h"
#include "lauxlib.h"

static int lmul(lua_State *L) {
	uint32_t c1 = (uint32_t)luaL_checkinteger(L, 1);
	uint32_t c2 = (uint32_t)luaL_checkinteger(L, 2);
	lua_pushinteger(L, color_mul(c1, c2));
	return 1;
}

static int ladd(lua_State *L) {
	uint32_t c1 = (uint32_t)luaL_checkinteger(L, 1);
	uint32_t c2 = (uint32_t)luaL_checkinteger(L, 2);
	lua_pushinteger(L, color_add(c1, c2));
	return 1;
}

static int lcolor(lua_State *L) {
	int n = lua_gettop(L);
	if (n == 4) {
		int r = (int)lua_tonumber(L, 1);
		int g = (int)lua_tonumber(L, 2);
		int b = (int)lua_tonumber(L, 3);
		int a = (int)lua_tonumber(L, 4);
		uint32_t color = (uint32_t)a << 24 | (uint32_t)r << 16 | (uint32_t)g << 8 | b;
		lua_pushinteger(L, color);
		return 1;
	} else {
		uint32_t color = (uint32_t)luaL_checkinteger(L, 1);
		int r = (color >> 16) & 0xff * 255;
		int g = (color >> 8) & 0xff * 255;
		int b = (color)& 0xff * 255;
		int a = (color >> 24) & 0xff * 255;
		lua_pushinteger(L, r);
		lua_pushinteger(L, g);
		lua_pushinteger(L, b);
		lua_pushinteger(L, a);
		return 4;
	}
}

int pixel_color(lua_State *L) {
	luaL_Reg l[] = {
		{"mul", lmul},
		{"add", ladd},
		{"color", lcolor},
		{0, 0},
	};
	luaL_newlib(L, l);
	return 1;
}

#endif // PIXEL_LUA