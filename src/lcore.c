#include "pixel.h"
#include "opengl.h"

#include "lua.h"
#include "lauxlib.h"

int luaopen_pixel_core(lua_State *L) {
	GLenum err;
	luaL_Reg l[] = {
		{ "viewport", pixel_viewport },
		{ "flush", pixel_flush },
		{ 0,0 },
	};
	err = glewInit();
	if (GLEW_OK != err) {
		return luaL_error(L, "glewInit err : %s", (char *)glewGetErrorString(err));
	}
	pixel_init(L);
	luaL_newlib(L, l);
	if (luaL_newmetatable(L, "pixel_core")) {
		lua_pushcfunction(L, pixel_unit);
		lua_setfield(L, -2, "__gc");
	}
	lua_setmetatable(L, -2);
	return 1;
}