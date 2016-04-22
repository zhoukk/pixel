
#define AOI_IMPLEMENTATION
#include "aoi.h"

#include "lua.h"
#include "lauxlib.h"

static struct aoi *aoi = 0;

static int lenter(lua_State *L) {
	int id = aoi_enter(aoi, 0);
	lua_pushinteger(L, id);
	return 1;
}

static int lleave(lua_State *L) {
	int id = (int)luaL_checkinteger(L, 1);
	aoi_leave(aoi, id);
	return 0;
}

static int llocate(lua_State *L) {
	int id = (int)luaL_checkinteger(L, 1);
	int x = (int)lua_tonumber(L, 2);
	int y = (int)lua_tonumber(L, 3);
	aoi_locate(aoi, id, x, y);
	return 0;
}

static int lmove(lua_State *L) {
	int id = (int)luaL_checkinteger(L, 1);
	int x = (int)lua_tonumber(L, 2);
	int y = (int)lua_tonumber(L, 3);
	aoi_move(aoi, id, x, y);
	return 0;
}

static int lmoving(lua_State *L) {
	int id = (int)luaL_checkinteger(L, 1);
	int moving = aoi_moving(aoi, id);
    lua_pushinteger(L, moving);
	return 1;
}

static int lpos(lua_State *L) {
	int id = (int)luaL_checkinteger(L, 1);
	int x, y;
	aoi_pos(aoi, id, &x, &y);
	lua_pushinteger(L, x);
	lua_pushinteger(L, y);
	return 2;
}

static int lupdate(lua_State *L) {
	int id = (int)luaL_checkinteger(L, 1);
	int tick = (int)luaL_checkinteger(L, 2);
	aoi_update(aoi, id, tick);
	return 0;
}

static int ltrigger(lua_State *L) {
	int id = (int)luaL_checkinteger(L, 1);
	int er = (int)lua_tonumber(L, 2);
	int lr = (int)lua_tonumber(L, 3);
    struct aoi_event *list;
	int r = aoi_trigger(aoi, id, er, lr, &list);
    lua_newtable(L);
    int i;
    for (i = 0; i < r; i++) {
        lua_pushinteger(L, i+1);
        lua_newtable(L);
        lua_pushstring(L, "id");
        lua_pushinteger(L, list[i].id);
        lua_settable(L, -3);
        lua_pushstring(L, "what");
        lua_pushinteger(L, list[i].e);
        lua_settable(L, -3);
        lua_settable(L, -3);
    }
	return 1;
}

static int laround(lua_State *L) {
	int id = (int)luaL_checkinteger(L, 1);
	int num = (int)luaL_checkinteger(L, 2);
    int list[num];
    int r = aoi_around(aoi, id, list, num);
    lua_newtable(L);
    int i;
    for (i = 0; i < r; i++) {
        lua_pushinteger(L, list[i]);
        lua_pushinteger(L, i+1);
        lua_settable(L, -3);
    }
    return 1;
}


static int lspeed(lua_State *L) {
	int id = (int)luaL_checkinteger(L, 1);
	int speed = (int)lua_tonumber(L, 2);
	aoi_speed(aoi, id, speed);
	return 0;
}

extern int luaopen_aoi(lua_State *L) {
	aoi = (struct aoi *)malloc(aoi_memsize());
	aoi_init(aoi);
    luaL_Reg l[] = {
        {"enter", lenter},
        {"leave", lleave},
        {"locate", llocate},
        {"move", lmove},
        {"moving", lmoving},
        {"pos", lpos},
        {"update", lupdate},
        {"around", laround},
        {"trigger", ltrigger},
        {"speed", lspeed},
        {0, 0},
    };
    luaL_newlib(L, l);
	return 1;
}
