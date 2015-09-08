#include "texture.h"
#include "pixel.h"
#include "readfile.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define MAX_TEXTURE 128

struct texture {
	int width;
	int height;
	float invw;
	float invh;
	int id;
	int fb;
};

struct texture_pool {
	struct texture texs[MAX_TEXTURE];
	int cur;
};

static struct texture_pool POOL;

void texture_init(void) {
	POOL.cur = 0;
	memset(POOL.texs, 0, MAX_TEXTURE*sizeof(struct texture));
}

void texture_unit(void) {
	int i;
	for (i = 0; i < POOL.cur; i++) {
		texture_unload(i);
	}
	POOL.cur = 0;
}

static inline uint32_t average4(uint32_t c[4]) {
	int i;
	uint32_t hi = 0;
	uint32_t low = 0;
	for (i = 0; i < 4; i++) {
		uint32_t v = c[i];
		low += v & 0x00ff00ff;
		hi += (v & 0xff00ff00) >> 8;
	}
	hi = (hi / 4) & 0x00ff00ff;
	low = (low / 4) & 0x00ff00ff;
	return hi << 8 | low;
}

static void texture_reduce(int *width, int *height, void *pixels) {
	int w = *width;
	int h = *height;
	uint32_t *src = (uint32_t *)pixels;
	char *dst = (char *)pixels;
	uint32_t average;
	int cnt = 0;
	int i, j;
	for (i = 0; i + 1 < h; i += 2) {
		for (j = 0; j + 1 < w; j += 2) {
			uint32_t c[4] = { src[j], src[j + 1], src[j + w], src[j + w + 1] };
			average = average4(c);
			dst[cnt] = average & 0xff;
			dst[cnt + 1] = (average >> 8) & 0xff;
			dst[cnt + 2] = (average >> 16) & 0xff;
			dst[cnt + 3] = (average >> 24) & 0xff;
			cnt += 4;
		}
		src += w * 2;
	}
	*width = w / 2;
	*height = h / 2;
}

int texture_load(int tid, enum TEXTURE_FORMAT t, int width, int height, void *pixels, int reduce) {
	struct texture *tex;
	if (tid >= MAX_TEXTURE) {
		return -1;
	}
	tex = &POOL.texs[tid];
	tex->fb = 0;
	tex->width = width;
	tex->height = height;
	tex->invw = 1.0f / (float)width;
	tex->invh = 1.0f / (float)height;
	if (tid >= POOL.cur) {
		POOL.cur = tid + 1;
	}
	if (tex->id == 0) {
		tex->id = render_texture_create(width, height, t, TEXTURE_2D, 0);
	}
	if (!pixels) {
		return tex->id;
	}
	if (t != TEXTURE_RGBA8) {
		reduce = 0;
	}
	if (reduce) {
		texture_reduce(&width, &height, pixels);
	}
	render_texture_update(tex->id, width, height, pixels, 0, 0);
	return tex->id;
}

int texture_loadfile(int tid, const char *filename, int reduce) {
	unsigned char *data, *pixels;
	int size, width, height, channel;
	int rid;
	enum TEXTURE_FORMAT t;
	data = (unsigned char *)readfile(filename, &size);
	if (!data) {
		return -1;
	}
	pixels = stbi_load_from_memory(data, size, &width, &height, &channel, 0);
	free(data);
	if (!pixels) {
		pixel_log("texture_loadfile %s err:%s\n", filename, stbi_failure_reason());
		return -1;
	}
	switch (channel) {
	case 1:
		t = TEXTURE_A8;
		break;
	case 2:
		t = TEXTURE_RGBA4;
		break;
	case 3:
		t = TEXTURE_RGB;
		break;
	case 4:
		t = TEXTURE_RGBA8;
		break;
	default:
		t = TEXTURE_INVALID;
		break;
	}
	rid = texture_load(tid, t, width, height, pixels, reduce);
	stbi_image_free(pixels);
	return rid;
}

int texture_rid(int tid) {
	if (tid < 0 || tid >= POOL.cur) {
		return 0;
	}
	return POOL.texs[tid].id;
}

void texture_size(int tid, int *width, int *height) {
	struct texture *tex;
	if (tid < 0 || tid >= POOL.cur) {
		*width = 0;
		*height = 0;
		return;
	}
	tex = &POOL.texs[tid];
	*width = tex->width;
	*height = tex->height;
}

int texture_update(int tid, int width, int height, void *pixels) {
	struct texture *tex;
	if (tid < 0 || tid >= POOL.cur) {
		return -1;
	}
	if (pixels == 0) {
		return -1;
	}
	tex = &POOL.texs[tid];
	if (tex->id == 0) {
		return -1;
	}
	render_texture_update(tex->id, width, height, pixels, 0, 0);
	tex->width = width;
	tex->height = height;
	tex->invw = 1.0f / (float)width;
	tex->invh = 1.0f / (float)height;
	return 0;
}

int texture_coord(int tid, float x, float y, uint16_t *u, uint16_t *v) {
	struct texture *tex;
	if (tid < 0 || tid >= POOL.cur) {
		*u = (uint16_t)x;
		*v = (uint16_t)y;
		return 1;
	}
	tex = &POOL.texs[tid];
	if (tex->invw == 0) {
		*u = (uint16_t)x;
		*v = (uint16_t)y;
		return 1;
	}
	x *= tex->invw;
	y *= tex->invh;
	if (x > 1.0f) {
		x = 1.0f;
	}
	if (y > 1.0f) {
		y = 1.0f;
	}
	x *= 0xffff;
	y *= 0xffff;
	*u = (uint16_t)x;
	*v = (uint16_t)y;
	return 0;
}

void texture_swap(int tida, int tidb) {
	struct texture tex;
	if (tida < 0 || tidb < 0 || tida >= POOL.cur || tidb >= POOL.cur) {
		return;
	}
	tex = POOL.texs[tida];
	POOL.texs[tida] = POOL.texs[tidb];
	POOL.texs[tidb] = tex;
}

void texture_unload(int tid) {
	struct texture *tex;
	if (tid < 0 || tid >= POOL.cur) {
		return;
	}
	tex = &POOL.texs[tid];
	if (tex->id == 0) {
		return;
	}
	render_rem(TEXTURE, tex->id);
	if (tex->fb != 0) {
		render_rem(TARGET, tex->fb);
	}
	tex->id = 0;
	tex->fb = 0;
}


#ifdef PIXEL_LUA

#include "lua.h"
#include "lauxlib.h"

static int lload(lua_State *L) {
	int tid = (int)luaL_checkinteger(L, 1);
	const char *file = luaL_checkstring(L, 2);
	int reduce = (int)luaL_optinteger(L, 3, 0);
	int rid = texture_loadfile(tid, file, reduce);
	if (rid == -1) {
		return 0;
	}
	lua_pushinteger(L, rid);
	return 1;
}

static int lunload(lua_State *L) {
	int tid = (int)luaL_checkinteger(L, 1);
	texture_unload(tid);
	return 0;
}

static int lsize(lua_State *L) {
	int tid = (int)luaL_checkinteger(L, 1);
	int width, height;
	texture_size(tid, &width, &height);
	lua_pushinteger(L, width);
	lua_pushinteger(L, height);
	return 2;
}

static int lswap(lua_State *L) {
	int tida = (int)luaL_checkinteger(L, 1);
	int tidb = (int)luaL_checkinteger(L, 2);
	texture_swap(tida, tidb);
	return 0;
}

int pixel_texture(lua_State *L) {
	luaL_Reg l[] = {
		{"load", lload},
		{"unload", lunload},
		{"size", lsize},
		{"swap", lswap},
		{0, 0},
	};
	luaL_newlib(L, l);
	return 1;
}
#endif // PIXEL_LUA
