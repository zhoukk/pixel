#ifndef _TEXTURE_H_
#define _TEXTURE_H_

#include "render.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

	void texture_init(void);
	void texture_unit(void);
	int texture_load(int tid, enum TEXTURE_FORMAT t, int width, int height, void *pixels, int reduce);
	int texture_loadfile(int tid, const char *filename, int reduce);
	void texture_unload(int tid);
	int texture_coord(int tid, float x, float y, uint16_t *u, uint16_t *v);
	int texture_rid(int tid);
	void texture_size(int tid, int *width, int *height);
	int texture_update(int tid, int width, int height, void *pixels);
	void texture_swap(int tida, int tidb);

#ifdef PIXEL_LUA
#include "lua.h"
	int pixel_texture(lua_State *L);
#endif // PIXEL_LUA

#ifdef __cplusplus
};
#endif
#endif // _TEXTURE_H_