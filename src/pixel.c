#include "pixel.h"
#include "shader.h"
#include "label.h"
#include "screen.h"
#include "texture.h"
#include "readfile.h"
#include "color.h"
#include "sprite.h"
#include "spritepack.h"
#include "particle.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


#ifdef PIXEL_LUA

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include "luaX_loader.h"

#define PIXEL_INIT		"PIXEL_INIT"
#define PIXEL_UNIT		"PIXEL_UNIT"
#define PIXEL_UPDATE	"PIXEL_UPDATE"
#define PIXEL_FRAME		"PIXEL_FRAME"
#define PIXEL_TOUCH		"PIXEL_TOUCH"
#define PIXEL_KEY		"PIXEL_KEY"
#define PIXEL_ERROR		"PIXEL_ERROR"
#define PIXEL_MESSAGE	"PIXEL_MESSAGE"
#define PIXEL_WHEEL		"PIXEL_WHEEL"
#define PIXEL_GESTURE 	"PIXEL_GESTURE"
#define PIXEL_PAUSE 	"PIXEL_PAUSE"
#define PIXEL_RESUME 	"PIXEL_RESUME"

#define PIXEL_FPS 30

struct pixel {
	lua_State *L;
	float w;
	float h;
	float s;
	int touch;
	int fps;
	int curfps;
	float real;
	float logic;
};

static struct pixel P;

static const char *start_script =
"local script = ...\n"
"package.path = 'lualib/?.lua;?.lua'\n"
"local f = assert(loadfile(script))\n"
"f(script)\n";

static int _panic(lua_State *L) {
	pixel_log("panic: %s\n", lua_tostring(L, -1));
	return 0;
}

static int traceback(lua_State *L) {
	const char *msg = lua_tostring(L, 1);
	if (msg) {
		luaL_traceback(L, L, msg, 1);
	} else if (!lua_isnoneornil(L, 1)) {
		if (!luaL_callmeta(L, 1, "__tostring")) {
			lua_pushliteral(L, "(no error message)");
		}
	}
	return 1;
}

static void error(lua_State *L, const char *err) {
	lua_getfield(L, LUA_REGISTRYINDEX, PIXEL_ERROR);
	lua_pushstring(L, err);
	if (LUA_OK != lua_pcall(L, 1, 0, 0)) {
		pixel_log("error: %s\n", lua_tostring(L, -1));
	}
}

static int call(lua_State *L, int n, int r) {
	int err = lua_pcall(L, n, r, 1);
	switch (err) {
	case LUA_OK:
		break;
	default:
		error(L, lua_tostring(L, -1));
		break;
	}
	return err;
}

static int linject(lua_State *L) {
	int i;
	static const char *pixel_cb[] = {
		PIXEL_INIT,
		PIXEL_UNIT,
		PIXEL_UPDATE,
		PIXEL_FRAME,
		PIXEL_TOUCH,
		PIXEL_KEY,
		PIXEL_ERROR,
		PIXEL_MESSAGE,
		PIXEL_WHEEL,
		PIXEL_GESTURE,
		PIXEL_PAUSE,
		PIXEL_RESUME,
	};
	for (i = 0; i < sizeof(pixel_cb) / sizeof(pixel_cb[0]); i++) {
		lua_getfield(L, lua_upvalueindex(1), pixel_cb[i]);
		if (!lua_isfunction(L, -1)) {
			return luaL_error(L, "%s not found", pixel_cb[i]);
		}
		lua_setfield(L, LUA_REGISTRYINDEX, pixel_cb[i]);
	}
	return 0;
}

static int llog(lua_State *L) {
	pixel_log("%s", lua_tostring(L, 1));
	return 0;
}

static int lfps(lua_State *L) {
	int fps = (int)luaL_optinteger(L, 1, 0);
	lua_pushinteger(L, pixel_fps(fps));
	return 1;
}

static int lfont(lua_State *L) {
	label_init(lua_tostring(L, 1));
	return 0;
}

static int lsize(lua_State *L) {
	float w = (float)lua_tonumber(L, 1);
	float h = (float)lua_tonumber(L, 2);
	float sw, sh;
	if (w == 0 || h == 0) {
		return 0;
	}
	sw = P.w / w;
	sh = P.h / h;
	if (sw > sh) {
		P.s = sw;
		h = P.h / P.s;
	} else {
		P.s = sh;
		w = P.w / P.s;
	}
	screen_init(w, h, P.s);
	return 0;
}

static int lread(lua_State *L) {
	const char *file = lua_tostring(L, 1);
	char *data;
	int size;
	data = readfile(file, &size);
	lua_pushlstring(L, data, size);
	lua_pushinteger(L, size);
	free(data);
	return 2;
}

static int pixel_framework(lua_State *L) {
	luaL_Reg l[] = {
		{ "inject", linject },
		{ "log", llog },
		{ "fps", lfps },
		{ "font", lfont },
		{ "size", lsize },
		{ "read", lread },
		{ 0, 0 },
	};
	luaL_newlibtable(L, l);
	lua_pushvalue(L, -1);
	luaL_setfuncs(L, l, 1);
	return 1;
}

int pixel_geometry(lua_State *L);

int pixel_init(lua_State *L) {
	luaL_checkversion(L);
	luaL_requiref(L, "pixel.framework", pixel_framework, 0);
	luaL_requiref(L, "pixel.texture", pixel_texture, 0);
	luaL_requiref(L, "pixel.shader", pixel_shader, 0);
	luaL_requiref(L, "pixel.renderbuffer", pixel_renderbuffer, 0);
	luaL_requiref(L, "pixel.matrix", pixel_matrix, 0);
	luaL_requiref(L, "pixel.color", pixel_color, 0);
	luaL_requiref(L, "pixel.geometry", pixel_geometry, 0);
	luaL_requiref(L, "pixel.sprite", pixel_sprite, 0);
	luaL_requiref(L, "pixel.spritepack", pixel_spritepack, 0);
	luaL_requiref(L, "pixel.particle", pixel_particle, 0);
	lua_settop(L, 0);
	shader_init();
	texture_init();
	label_init(0);
	return 0;
}

int pixel_unit(lua_State *L) {
	(void)L;
	label_unit();
	texture_unit();
	shader_unit();
	return 0;
}

int pixel_flush(lua_State *L) {
	(void)L;
	shader_flush();
	label_flush();
	return 0;
}

int pixel_viewport(lua_State *L) {
	float w = (float)lua_tonumber(L, 1);
	float h = (float)lua_tonumber(L, 2);
	float s = (float)luaL_optnumber(L, 3, 1.0f);
	screen_init(w, h, s);
	return 0;
}

void pixel_close(void) {
	lua_State *L = P.L;
	lua_getfield(L, LUA_REGISTRYINDEX, PIXEL_UNIT);
	call(L, 0, 0);
	lua_settop(L, 3);

	pixel_unit(L);

	lua_close(L);
}

void pixel_start(int w, int h, const char *start) {
	lua_State *L = luaL_newstate();
	luaL_checkversion(L);
	lua_atpanic(L, _panic);
	luaL_openlibs(L);
	luaX_loader(L);

	pixel_init(L);

	pixel_log("version:%d\n", shader_version());

	memset(&P, 0, sizeof P);
	P.L = L;
	P.fps = PIXEL_FPS;
	P.w = w;
	P.h = h;
	P.s = 1.0f;
	screen_init(P.w, P.h, P.s);
	lua_pushcfunction(L, traceback);
	int tb = lua_gettop(L);
	if (LUA_OK != luaL_loadstring(L, start_script)) {
		pixel_log("%s\n", lua_tostring(L, -1));
	}
	lua_pushstring(L, start);
	if (LUA_OK != lua_pcall(L, 1, 0, tb)) {
		pixel_log("lua_init:%s\n", lua_tostring(L, -1));
	}
	lua_pop(L, 1);
	lua_getfield(L, LUA_REGISTRYINDEX, PIXEL_INIT);
	lua_call(L, 0, 0);
	assert(lua_gettop(L) == 0);
	lua_pushcfunction(L, traceback);
	lua_getfield(L, LUA_REGISTRYINDEX, PIXEL_UPDATE);
	lua_getfield(L, LUA_REGISTRYINDEX, PIXEL_FRAME);
}

static void calc_fps(float t) {
	static int frames = 0;
	static float time_elapsed = 0.0f;

	frames++;
	if (time_elapsed >= 1.0f) {
		P.curfps = frames;
		time_elapsed = 0;
		frames = 0;
	} else {
		time_elapsed += t;
	}
}

void pixel_frame(float t) {
	lua_State *L = P.L;
	lua_pushvalue(L, 3);
	call(L, 0, 0);
	lua_settop(L, 3);
	calc_fps(t);
	pixel_flush(L);
}

void pixel_update(float t) {
	if (P.fps <= 0) return;
	if (P.logic == 0) P.real = 1.0f / P.fps;
	else P.real += t;
	while (P.logic < P.real) {
		lua_State *L = P.L;
		lua_pushvalue(L, 2);
		call(L, 0, 0);
		lua_settop(L, 3);
		P.logic += 1.0f / P.fps;
	}
}

void pixel_touch(int id, int what, int x, int y) {
	if (what == TOUCH_BEGIN) P.touch = 1;
	if (P.touch == 1) {
		lua_State *L = P.L;
		lua_getfield(L, LUA_REGISTRYINDEX, PIXEL_TOUCH);
		lua_pushinteger(L, what);
		lua_pushinteger(L, (int)(x / P.s));
		lua_pushinteger(L, (int)(y / P.s));
		lua_pushinteger(L, id);
		call(L, 4, 0);
		lua_settop(L, 3);
	}
	if (what == TOUCH_END) {
		P.touch = 0;
	}
}

void pixel_key(int what, char ch) {
	lua_State *L = P.L;
	lua_getfield(L, LUA_REGISTRYINDEX, PIXEL_KEY);
	lua_pushinteger(L, what);
	lua_pushinteger(L, ch);
	call(L, 2, 0);
	lua_settop(L, 3);
}

void pixel_wheel(int delta, int x, int y) {
	lua_State *L = P.L;
	lua_getfield(L, LUA_REGISTRYINDEX, PIXEL_WHEEL);
	lua_pushinteger(L, delta);
	lua_pushinteger(L, (int)(x / P.s));
	lua_pushinteger(L, (int)(y / P.s));
	call(L, 3, 0);
	lua_settop(L, 3);
}

void pixel_gesture(int type, int x1, int y1, int x2, int y2, int s) {
	lua_State *L = P.L;
	lua_getfield(L, LUA_REGISTRYINDEX, PIXEL_GESTURE);
	lua_pushinteger(L, type);
	lua_pushinteger(L, (int)(x1 / P.s));
	lua_pushinteger(L, (int)(y1 / P.s));
	lua_pushinteger(L, (int)(x2 / P.s));
	lua_pushinteger(L, (int)(y2 / P.s));
	lua_pushinteger(L, s);
	call(L, 6, 0);
	lua_settop(L, 3);
}

void pixel_pause(void) {
	lua_State *L = P.L;
	lua_getfield(L, LUA_REGISTRYINDEX, PIXEL_PAUSE);
	call(L, 0, 0);
	lua_settop(L, 3);
}

void pixel_resume(void) {
	lua_State *L = P.L;
	lua_getfield(L, LUA_REGISTRYINDEX, PIXEL_RESUME);
	call(L, 0, 0);
	lua_settop(L, 3);
}

void pixel_message(const char *msg) {
	lua_State *L = P.L;
	lua_getfield(L, LUA_REGISTRYINDEX, PIXEL_MESSAGE);
	lua_pushstring(L, msg);
	call(L, 1, 0);
	lua_settop(L, 3);
}

int pixel_fps(int fps) {
	if (fps > 0) {
		int ofps = P.fps;
		P.fps = fps;
		return ofps;
	}
	return P.curfps;
}

#endif // PIXEL_LUA
