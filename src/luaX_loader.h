#ifndef luaX_loadfile_h
#define luaX_loadfile_h
#ifdef __cplusplus
extern "C"{
#endif
#include "lua.h"
#include "lauxlib.h"

#include <string.h>

#if !defined (LUA_PATH_SEP)
#define LUA_PATH_SEP		";"
#endif
#if !defined (LUA_PATH_MARK)
#define LUA_PATH_MARK		"?"
#endif
#if !defined (LUA_EXEC_DIR)
#define LUA_EXEC_DIR		"!"
#endif
#if !defined (LUA_IGMARK)
#define LUA_IGMARK		"-"
#endif
#if !defined(LUA_LSUBSEP)
#define LUA_LSUBSEP		LUA_DIRSEP
#endif

static const char *pushnexttemplate (lua_State *L, const char *path) {
	const char *l;
	while (*path == *LUA_PATH_SEP) path++;  /* skip separators */
	if (*path == '\0') return NULL;  /* no more templates */
	l = strchr(path, *LUA_PATH_SEP);  /* find next separator */
	if (l == NULL) l = path + strlen(path);
	lua_pushlstring(L, path, l - path);  /* template */
	return l;
}


static const char *searchpath (lua_State *L, const char *name,
	const char *path,
	const char *sep,
	const char *dirsep) {
		luaL_Buffer msg;  /* to build error message */
		luaL_buffinit(L, &msg);
		if (*sep != '\0')  /* non-empty separator? */
			name = luaL_gsub(L, name, sep, dirsep);  /* replace it by 'dirsep' */
		while ((path = pushnexttemplate(L, path)) != NULL) {
			const char *filename = luaL_gsub(L, lua_tostring(L, -1),
				LUA_PATH_MARK, name);
			lua_remove(L, -2);  /* remove path template */
			if (readable(filename))  /* does file exist and is readable? */
				return filename;  /* return that file name */
			lua_pushfstring(L, "\n\tno file '%s'", filename);
			lua_remove(L, -2);  /* remove file name */
			luaL_addvalue(&msg);  /* concatenate error msg. entry */
		}
		luaL_pushresult(&msg);  /* create error message */
		return NULL;  /* not found */
}

static const char *findfile (lua_State *L, const char *name,
	const char *pname,
	const char *dirsep) {
		const char *path;
		lua_getfield(L, lua_upvalueindex(1), pname);
		path = lua_tostring(L, -1);
		if (path == NULL)
			luaL_error(L, "'package.%s' must be a string", pname);
		return searchpath(L, name, path, ".", dirsep);
}


static int checkload (lua_State *L, int stat, const char *filename) {
	if (stat) {  /* module loaded successfully? */
		lua_pushstring(L, filename);  /* will be 2nd argument to module */
		return 2;  /* return open function and file name */
	}
	else
		return luaL_error(L, "error loading module '%s' from file '%s':\n\t%s",
		lua_tostring(L, 1), filename, lua_tostring(L, -1));
}

int luaX_loadfile(lua_State *L, const char *filename);
static int luaX_searcher(lua_State *L) {
	const char *filename;
	const char *name = luaL_checkstring(L, 1);
	filename = findfile(L, name, "path", LUA_LSUBSEP);
	if (filename == NULL) return 1;  /* module not found in this path */
	return checkload(L, (luaX_loadfile(L, filename) == LUA_OK), filename);
}

typedef struct LoadF {
	char *filebuff;
	char *buff;
	char *p;
	int size;
} LoadF;

static const char *getF (lua_State *L, void *ud, size_t *size) {
  LoadF *lf = (LoadF *)ud;
  (void)L;  /* not used */
  if (lf->size > 0) {  /* are there pre-read characters to be read? */
    *size = lf->size;  /* return them (chars already in buffer) */
	lf->buff = lf->filebuff;
    lf->size = 0;  /* no more pre-read characters */
  }
  else {  /* read a block from file */
    /* 'fread' can return > 0 *and* set the EOF flag. If next call to
       'getF' called 'fread', it might still wait for user input.
       The next check avoids this problem. */
    if (lf->size == 0) return NULL;
    *size = LUAL_BUFFERSIZE;
	lf->buff = lf->filebuff;
	lf->filebuff += LUAL_BUFFERSIZE;
	lf->size -= LUAL_BUFFERSIZE;
  }
  return lf->buff;
}

int luaX_loadfile(lua_State *L, const char *filename) {
	LoadF lf;
	int status;
	lf.filebuff = readfile(filename, &lf.size);
	lf.p = lf.filebuff;
	if (!lf.filebuff) {
		return -1;
	}
	status = lua_load(L, getF, &lf, lua_tostring(L, -1), 0);
	free(lf.p);
	return status;
}

static int load_aux (lua_State *L, int status, int envidx) {
	if (status == LUA_OK) {
		if (envidx != 0) {  /* 'env' parameter? */
			lua_pushvalue(L, envidx);  /* environment for loaded function */
			if (!lua_setupvalue(L, -2, 1))  /* set it as 1st upvalue */
				lua_pop(L, 1);  /* remove 'env' if not used by previous call */
		}
		return 1;
	}
	else {  /* error (message is on top of the stack) */
		lua_pushnil(L);
		lua_insert(L, -2);  /* put before error message */
		return 2;  /* return nil plus error message */
	}
}

static int lluaX_loadfile (lua_State *L) {
	const char *fname = luaL_optstring(L, 1, NULL);
	const char *mode = luaL_optstring(L, 2, NULL);
	int env = (!lua_isnone(L, 3) ? 3 : 0);  /* 'env' index or 0 if no 'env' */
	int status = luaX_loadfile(L, fname);
	(void)mode;
	return load_aux(L, status, env);
}

static int dofilecont (lua_State *L, int d1, lua_KContext d2) {
	(void)d1;  (void)d2;  /* only to match 'lua_Kfunction' prototype */
	return lua_gettop(L) - 1;
}

static int lluaX_dofile (lua_State *L) {
	const char *fname = luaL_optstring(L, 1, NULL);
	lua_settop(L, 1);
	if (luaX_loadfile(L, fname) != LUA_OK)
		return lua_error(L);
	lua_callk(L, 0, LUA_MULTRET, 0, dofilecont);
	return dofilecont(L, 0, 0);
}

int luaX_dofile(lua_State *L, const char *filename) {
	if (luaX_loadfile(L, filename) != LUA_OK)
		return lua_error(L);
	lua_callk(L, 0, LUA_MULTRET, 0, dofilecont);
	return dofilecont(L, 0, 0);
}

int luaX_loader(lua_State *L) {
	lua_getglobal(L, "_G");
	if (lua_istable(L, -1)) {
		lua_pushcfunction(L, lluaX_loadfile);
		lua_setfield(L, -2, "loadfile");
		lua_pushcfunction(L, lluaX_dofile);
		lua_setfield(L, -2, "dofile");
	}
	lua_getglobal(L, LUA_LOADLIBNAME);
	if (lua_istable(L, -1)) {
		lua_getfield(L, -1, "searchers");
		if (lua_istable(L, -1)) {
			lua_pushvalue(L, -2);
			lua_pushcclosure(L, luaX_searcher, 1);
			lua_rawseti(L, -2, 2);
			lua_pop(L, 2);
			return 0;
		}
	}
	return -1;
}
#ifdef __cplusplus
};
#endif
#endif // luaX_loadfile_h
