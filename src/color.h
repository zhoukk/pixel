#ifndef _COLOR_H_
#define _COLOR_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

	uint32_t color_mul(uint32_t c1, uint32_t c2);
	uint32_t color_add(uint32_t c1, uint32_t c2);

#ifdef PIXEL_LUA
#include "lua.h"
	int pixel_color(lua_State *L);
#endif // PIXEL_LUA

#ifdef __cplusplus
};
#endif
#endif // _COLOR_H_
