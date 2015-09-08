#include "spritepack.h"
#include "hash.h"
#include "texture.h"
#include "matrix.h"
#include "shader.h"
#include "stream.h"
#include "pixel.h"
#include "readfile.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <malloc.h>

#define PACK_SIZE 16

struct slloc {
	char *data;
	int size;
};

static void *plloc(void *ud, int size) {
	void *data;
	struct slloc *slloc = (struct slloc *)ud;

	assert(slloc->size >= size);
	data = slloc->data;
	slloc->data += size;
	slloc->size -= size;
	return data;
}

struct spritepack {
	int *tex;
	int matrix_n;
	struct matrix *matrix;
	struct slloc slloc;
	struct stream is;
	struct sprite_pack *p;
	struct hash *h;
	int *export;
};

struct spritepack_storage {
	struct hash *h;
	struct spritepack *array;
	int n;
	int tex;
	const char *path;
};

static struct spritepack_storage S = { 0,0,0 };

static void _import_matrix(struct spritepack *sp) {
	int n, i;

	n = stream_r32(&sp->is);
	sp->matrix = (struct matrix *)plloc(&sp->slloc, n * sizeof(struct matrix));
	sp->matrix_n = n;
	for (i = 0; i < n; i++) {
		int j;
		struct matrix *mat = &sp->matrix[i];
		for (j = 0; j < 6; j++) {
			mat->m[j] = stream_r32(&sp->is);
		}
	}
}

static void _import_picture(struct spritepack *sp) {
	int i, n;
	struct pack_picture *pp;

	n = stream_r8(&sp->is);
	pp = (struct pack_picture *)plloc(&sp->slloc, sizeof(struct pack_picture) + (n - 1)*sizeof(struct pack_quad));
	pp->n = n;
	for (i = 0; i < n; i++) {
		int j;
		struct pack_quad *q = &pp->rect[i];
		int texid = stream_r8(&sp->is);
		q->texid = sp->tex[texid];
		for (j = 0; j < 8; j += 2) {
			float x = (float)stream_r16(&sp->is);
			float y = (float)stream_r16(&sp->is);
			texture_coord(q->texid, x, y, &q->texture_coord[j], &q->texture_coord[j + 1]);
		}
		for (j = 0; j < 8; j++) {
			q->screen_coord[j] = stream_r32(&sp->is);
		}
	}
}

static void _import_frame(struct spritepack *sp, struct pack_frame *pf, int component_n) {
	int i;
	int n = stream_r16(&sp->is);
	pf->part = (struct pack_part *)plloc(&sp->slloc, n*sizeof(struct pack_part));
	pf->n = n;
	for (i = 0; i < n; i++) {
		struct pack_part *pp = &pf->part[i];
		int tag = stream_r8(&sp->is);
		if (tag & TAG_ID) {
			pp->component_id = stream_r16(&sp->is);
			assert(pp->component_id < component_n);
		} else {
			assert(0);
		}
		if (tag & TAG_MATRIX) {
			int j;
			int32_t *m;
			pp->t.mat = (struct matrix *)plloc(&sp->slloc, sizeof(struct matrix));
			m = pp->t.mat->m;
			for (j = 0; j < 6; j++) {
				m[j] = stream_r32(&sp->is);
			}
		} else if (tag & TAG_MATRIXREF) {
			int ref = stream_r32(&sp->is);
			pp->t.mat = &sp->matrix[ref];
		} else {
			pp->t.mat = 0;
		}
		if (tag & TAG_COLOR) {
			pp->t.color = stream_r32(&sp->is);
		} else {
			pp->t.color = 0xffffffff;
		}
		if (tag & TAG_ADDITIVE) {
			pp->t.addi = stream_r32(&sp->is);
		} else {
			pp->t.addi = 0;
		}
		if (tag & TAG_TOUCH) {
			stream_r16(&sp->is);
			pp->touchable = 1;
		} else {
			pp->touchable = 0;
		}
		pp->t.pid = PROGRAM_DEFAULT;
	}
}

static void _import_animation(struct spritepack *sp) {
	int i;
	int frame = 0;
	int component_n = stream_r16(&sp->is);
	int size = sizeof(struct pack_animation) + (component_n - 1)*sizeof(struct pack_component);
	struct pack_animation *pa = (struct pack_animation *)plloc(&sp->slloc, size);
	pa->component_n = component_n;
	for (i = 0; i < component_n; i++) {
		int id = stream_r16(&sp->is);
		pa->component[i].id = id;
		pa->component[i].name = (const uint8_t *)stream_rstr(&sp->is, 0, plloc, &sp->slloc);
	}
	pa->action_n = stream_r16(&sp->is);
	pa->action = (struct pack_action *)plloc(&sp->slloc, pa->action_n*sizeof(struct pack_action));
	for (i = 0; i < pa->action_n; i++) {
		pa->action[i].name = (const uint8_t *)stream_rstr(&sp->is, 0, plloc, &sp->slloc);
		pa->action[i].n = stream_r16(&sp->is);
		pa->action[i].start_frame = frame;
		frame += pa->action[i].n;
	}
	pa->frame_n = stream_r16(&sp->is);
	pa->frame = (struct pack_frame *)plloc(&sp->slloc, pa->frame_n*sizeof(struct pack_frame));
	for (i = 0; i < pa->frame_n; i++) {
		_import_frame(sp, &pa->frame[i], component_n);
	}
}

static void _import_label(struct spritepack *sp) {
	struct pack_label *pl = (struct pack_label *)plloc(&sp->slloc, sizeof(struct pack_label));
	pl->align = stream_r8(&sp->is);
	pl->color = stream_r32(&sp->is);
	pl->size = stream_r16(&sp->is);
	pl->width = stream_r16(&sp->is);
	pl->height = stream_r16(&sp->is);
	pl->edge = stream_r8(&sp->is);
	pl->space_w = stream_r8(&sp->is);
	pl->space_h = stream_r8(&sp->is);
	pl->auto_scale = stream_r8(&sp->is);
}

static void _import_panel(struct spritepack *sp) {
	struct pack_panel *pp = (struct pack_panel *)plloc(&sp->slloc, sizeof(struct pack_panel));
	pp->width = stream_r32(&sp->is);
	pp->height = stream_r32(&sp->is);
	pp->scissor = stream_r8(&sp->is);
}

static void _import_polygon(struct spritepack *sp) {
	int i, j;
	int n = stream_r8(&sp->is);
	struct pack_polygon *pp = (struct pack_polygon *)plloc(&sp->slloc, sizeof(struct pack_polygon) + (n - 1)*sizeof(struct pack_poly));
	pp->n = n;
	for (i = 0; i < n; i++) {
		struct pack_poly *p = &pp->poly[i];
		int tid = stream_r8(&sp->is);
		p->texid = sp->tex[tid];
		p->n = stream_r8(&sp->is);
		p->texture_coord = (uint16_t *)plloc(&sp->slloc, p->n * 2 * sizeof(uint16_t));
		p->screen_coord = (int32_t *)plloc(&sp->slloc, p->n * 2 * sizeof(int32_t));
		for (j = 0; j < p->n * 2; j += 2) {
			float x = (float)stream_r16(&sp->is);
			float y = (float)stream_r16(&sp->is);
			texture_coord(p->texid, x, y, &p->texture_coord[j], &p->texture_coord[j + 1]);
		}
		for (j = 0; j < p->n * 2; j++) {
			p->screen_coord[j] = stream_r32(&sp->is);
		}
	}
}

void _import_sprite(struct spritepack *sp) {
	int id, type;

	id = stream_r16(&sp->is);
	type = stream_r8(&sp->is);
	if (type == TYPE_MATRIX) {
		_import_matrix(sp);
		return;
	}
	sp->p->type[id] = type;
	sp->p->data[id] = sp->slloc.data;
	switch (type) {
	case TYPE_PICTURE:
		_import_picture(sp);
		break;
	case TYPE_ANIMATION:
		_import_animation(sp);
		break;
	case TYPE_POLYGON:
		_import_polygon(sp);
		break;;
	case TYPE_LABEL:
		_import_label(sp);
		break;
	case TYPE_PANEL:
		_import_panel(sp);
		break;
	default:
		pixel_log("_import_sprite type:%d err\n", type);
		break;
	}
}

static int _require_tex(const char *file, int i, const char *ext) {
	char tmp[256];
	sprintf(tmp, "%s%s.%d.%s", S.path, file, i, ext);
	if (-1 == texture_loadfile(S.tex, tmp, 0)) {
		return -1;
	}
	return S.tex++;
}

static void *alloc(void *ud, int size) {
	return ud;
}

int spritepack_size(void) {
	return sizeof(struct sprite_pack);
}

static void spritepack_import(struct spritepack *sp, int *texture, int maxid, char *packdata, int packsize, char *metadata, int metasize) {
	stream_init(&sp->is, metadata, metasize);
	sp->slloc.data = packdata;
	sp->slloc.size = packsize;

	sp->p = (struct sprite_pack *)plloc(&sp->slloc, sizeof(struct sprite_pack));
	sp->p->n = maxid + 1;
	sp->tex = texture;
	sp->p->data = (void **)plloc(&sp->slloc, sp->p->n*sizeof(void *));
	memset(sp->p->data, 0, sp->p->n*sizeof(void *));
	sp->p->type = (int *)plloc(&sp->slloc, sp->p->n*sizeof(int));
	memset(sp->p->type, 0, sp->p->n*sizeof(int));

	for (; sp->is.size > 0;) {
		_import_sprite(sp);
	}
}

void spritepack_init(const char *path) {
	S.path = path;
	S.n = 16;
	S.h = hash_new(S.n);
	S.array = malloc(S.n * sizeof(struct spritepack));
	memset(S.array, 0, S.n*sizeof(struct spritepack));
}

void spritepack_unit(void) {
	int i;
	hash_free(S.h);
	for (i = 0; i < S.n; i++) {
		struct spritepack *sp = &S.array[i];
		if (sp->p) {
			free(sp->p);
		}
		if (sp->h) {
			hash_free(sp->h);
		}
		if (sp->export) {
			free(sp->export);
		}
	}
	free(S.array);
}

struct sprite_pack *spritepack_load(const char *file) {
	int size, texture_n, export_n, packsize, metasize;
	char *data, *metadata, *packdata;
	int i, maxid;
	char tmp[256];
	struct stream is;
	struct spritepack *sp;
	long pos;

	pos = hash_insert(S.h, file, strlen(file));
	if (pos == -1) {
		return 0;
	}
	sp = &S.array[pos];

	sprintf(tmp, "%s%s.pi", S.path, file);
	data = readfile(tmp, &size);
	if (!data) {
		pixel_log("spritepack_load:%s failed\n", tmp);
		return 0;
	}
	is.data = data;
	is.size = size;

	export_n = stream_r16(&is);
	maxid = stream_r16(&is);
	texture_n = stream_r16(&is);
	packsize = stream_r32(&is);
	stream_r32(&is);

	sp->export = malloc(export_n * sizeof(int));
	sp->h = hash_new(export_n);
	for (i = 0; i < export_n; i++) {
		char buff[1024];
		int len;
		int id = stream_r16(&is);
		const char *name = stream_rstr(&is, &len, alloc, buff);
		pos = hash_insert(sp->h, name, strlen(name));
		sp->export[pos] = id;
	}
	packdata = (char *)malloc(packsize);
	metadata = is.data;
	metasize = is.size;
	if (1) {
#if defined(_MSC_VER)
		int *texture = (int *)_alloca(texture_n * sizeof(int));
#else
		int texture[texture_n];
#endif
		for (i = 0; i < texture_n; i++) {
			texture[i] = _require_tex(file, i + 1, "png");
		}
		spritepack_import(sp, texture, maxid, packdata, packsize, metadata, metasize);
	}
	free(data);
	pixel_log("spritepack_load:%s ok\n", file);
	return (struct sprite_pack *)packdata;
}

struct sprite_pack *spritepack_query(const char *file) {
	long pos = hash_exist(S.h, file, strlen(file));
	if (-1 != pos) {
		return S.array[pos].p;
	}
	return 0;
}

int spritepack_id(const char *file, const char *name) {
	long pos = hash_exist(S.h, file, strlen(file));
	if (-1 != pos) {
		struct spritepack *sp = &S.array[pos];
		pos = hash_exist(sp->h, name, strlen(name));
		if (-1 != pos) {
			return sp->export[pos];
		}
	}
	return -1;
}

#ifdef PIXEL_LUA

#include "lua.h"
#include "lauxlib.h"

static int32_t readinteger(lua_State *L, int idx) {
	if (lua_isinteger(L, idx)) {
		return (int32_t)lua_tointeger(L, idx);
	} else {
		lua_Number n = luaL_checknumber(L, idx);
		return (int32_t)n;
	}
}

static int lpackbyte(lua_State *L) {
	int n = (int)readinteger(L, 1);
	uint8_t buf[1] = { (uint8_t)n };
	lua_pushlstring(L, (char *)buf, 1);
	return 1;
}

static int lpackword(lua_State *L) {
	int n = (int)readinteger(L, 1);
	uint8_t buf[2] = {
		(uint8_t)n & 0xff ,
		(uint8_t)((n >> 8) & 0xff) ,
	};
	lua_pushlstring(L, (char *)buf, 2);
	return 1;
}

static int lpackint32(lua_State *L) {
	int32_t sn = (int32_t)readinteger(L, 1);
	uint32_t n = (uint32_t)sn;
	uint8_t buf[4] = {
		(uint8_t)n & 0xff ,
		(uint8_t)((n >> 8) & 0xff) ,
		(uint8_t)((n >> 16) & 0xff) ,
		(uint8_t)((n >> 24) & 0xff) ,
	};
	lua_pushlstring(L, (char *)buf, 4);
	return 1;
}

static int lpackcolor(lua_State *L) {
	uint32_t n = (uint32_t)luaL_checkinteger(L, 1);
	uint8_t buf[4] = {
		(uint8_t)n & 0xff ,
		(uint8_t)((n >> 8) & 0xff) ,
		(uint8_t)((n >> 16) & 0xff) ,
		(uint8_t)((n >> 24) & 0xff) ,
	};
	lua_pushlstring(L, (char *)buf, 4);
	return 1;
}

static int lpackstring(lua_State *L) {
	size_t sz = 0;
	const char *str = lua_tolstring(L, 1, &sz);
	if (sz >= 255) {
		return luaL_error(L, "%s is too long", str);
	}
	if (str == NULL) {
		uint8_t buf[1] = { 255 };
		lua_pushlstring(L, (char *)buf, 1);
	} else {
#if defined(_MSC_VER)
		uint8_t *buf = _alloca(sz + 1);
#else
		uint8_t buf[sz + 1];
#endif
		buf[0] = (uint8_t)sz;
		memcpy(buf + 1, str, sz);
		lua_pushlstring(L, (char *)(uint8_t *)buf, sz + 1);
	}
	return 1;
}

static int lpackframetag(lua_State *L) {
	const char * tagstr = luaL_checkstring(L, 1);
	int i;
	int tag = 0;
	uint8_t buf[1];
	for (i = 0; tagstr[i]; i++) {
		switch (tagstr[i]) {
		case 'i':
			tag |= TAG_ID;
			break;
		case 'c':
			tag |= TAG_COLOR;
			break;
		case 'a':
			tag |= TAG_ADDITIVE;
			break;
		case 'm':
			tag |= TAG_MATRIX;
			break;
		case 't':
			tag |= TAG_TOUCH;
			break;
		case 'M':
			tag |= TAG_MATRIXREF;
			break;
		default:
			return luaL_error(L, "Invalid tag %s", tagstr);
			break;
		}
	}
	buf[0] = (uint8_t)tag;
	lua_pushlstring(L, (char *)buf, 1);
	return 1;
}

static int lpicture_size(lua_State *L) {
	int n = (int)luaL_checkinteger(L, 1);
	int sz = sizeof(struct pack_picture) + (n - 1) * sizeof(struct pack_quad);
	lua_pushinteger(L, sz);
	return 1;
}

static int lpolygon_size(lua_State *L) {
	int n = (int)luaL_checkinteger(L, 1);
	int pn = (int)luaL_checkinteger(L, 2);
	int sz = sizeof(struct pack_polygon)
		+ (n - 1) * sizeof(struct pack_poly)
		+ 12 * pn;
	lua_pushinteger(L, sz);
	return 1;
}

static int lpack_size(lua_State *L) {
	int max_id = (int)luaL_checkinteger(L, 1);
	int tex = (int)luaL_checkinteger(L, 2);
	int size = sizeof(struct sprite_pack)
		+ (max_id + 1) * sizeof(void *)
		+ (max_id + 1) * sizeof(int)
		+ tex * sizeof(int);
	lua_pushinteger(L, size);
	return 1;
}

static int lanimation_size(lua_State *L) {
	int frame = (int)luaL_checkinteger(L, 1);
	int component = (int)luaL_checkinteger(L, 2);
	int action = (int)luaL_checkinteger(L, 3);

	int size = sizeof(struct pack_animation)
		+ frame * sizeof(struct pack_frame)
		+ action * sizeof(struct pack_action)
		+ (component - 1) * sizeof(struct pack_component);
	lua_pushinteger(L, size);
	return 1;
}

static int lpart_size(lua_State *L) {
	int size;
	if (lua_istable(L, 1)) {
		size = sizeof(struct pack_part) + sizeof(struct matrix);
	} else {
		size = sizeof(struct pack_part);
	}
	lua_pushinteger(L, size);
	return 1;
}

static int lstring_size(lua_State *L) {
	int size;
	if (lua_isnoneornil(L, 1)) {
		size = 0;
	} else {
		size_t sz = 0;
		luaL_checklstring(L, 1, &sz);
		size = (sz + 1 + 3) & ~3;
	}
	lua_pushinteger(L, size);
	return 1;
}

static int llabel_size(lua_State *L) {
	lua_pushinteger(L, sizeof(struct pack_label));
	return 1;
}

static int lpanel_size(lua_State *L) {
	lua_pushinteger(L, sizeof(struct pack_panel));
	return 1;
}

static int lnew(lua_State *L) {
	int texture_n;
	int t;
	char *packdata;
	char *metadata;
	size_t metasize;
	int maxid = (int)luaL_checkinteger(L, 2);
	int packsize = (int)luaL_checkinteger(L, 3);
	t = lua_type(L, 1);
	if (t == LUA_TNIL) {
		texture_n = 0;
	} else if (t == LUA_TNUMBER) {
		texture_n = 1;
	} else {
		if (t != LUA_TTABLE) {
			return luaL_error(L, "need texture");
		}
		texture_n = lua_rawlen(L, 1);
	}
	if (lua_isstring(L, 4)) {
		metadata = (char *)lua_tolstring(L, 4, &metasize);
	} else {
		metadata = (char *)lua_touserdata(L, 4);
		metasize = (size_t)luaL_checkinteger(L, 5);
	}
	packdata = (char *)lua_newuserdata(L, packsize);
	if (metadata) {
#if defined(_MSC_VER)
		int *texture = _alloca(texture_n *sizeof(int));
#else
		int texture[texture_n];
#endif
		struct spritepack sp;
		memset(&sp, 0, sizeof(sp));
		if (t == LUA_TTABLE) {
			int i;
			for (i = 0; i < texture_n; i++) {
				lua_rawgeti(L, 1, i + 1);
				texture[i] = (int)luaL_checkinteger(L, -1);
				lua_pop(L, 1);
			}
		} else if (t == LUA_TNUMBER) {
			texture[0] = (int)lua_tointeger(L, 1);
		}
		spritepack_import(&sp, texture, maxid, packdata, packsize, metadata, (int)metasize);
	} else {
		return luaL_error(L, "need metadata");
	}
	return 1;
}

static int limport(lua_State *L) {
	size_t sz = 0;
	const uint8_t *data = (const uint8_t *)luaL_checklstring(L, 1, &sz);
	int off = luaL_checkinteger(L, 2) - 1;
	int len = 0;
	const char *type = luaL_checkstring(L, 3);
	switch (type[0]) {
	case 'w':
	{
		len = 2;
		if (off + len > sz) {
			return luaL_error(L, "Invalid data stream");
		}
		int v = data[off] | data[off + 1] << 8;
		lua_pushinteger(L, v);
		break;
	}
	case 'i':
	{
		len = 4;
		if (off + len > sz) {
			return luaL_error(L, "Invalid data stream");
		}
		uint32_t v = data[off]
			| data[off + 1] << 8
			| data[off + 2] << 16
			| data[off + 3] << 24;
		lua_pushinteger(L, v);
		break;
	}
	case 's':
	{
		len = 1;
		if (off + len > sz) {
			return luaL_error(L, "Invalid data stream");
		}
		int slen = data[off];
		if (slen == 0xff) {
			lua_pushlstring(L, "", 0);
			break;
		}
		if (off + len + slen > sz) {
			return luaL_error(L, "Invalid data stream");
		}
		lua_pushlstring(L, (const char *)(data + off + 1), slen);
		len += slen;
		break;
	}
	case 'p':
	{
		len = 0;
		lua_pushlightuserdata(L, (void *)(data + off));
		break;
	}
	default:
		return luaL_error(L, "Invalid type %s", type);
	}
	lua_pushinteger(L, off + len + 1);
	return 2;
}

int pixel_spritepack(lua_State *L) {
	luaL_Reg l[] = {
		{ "new", lnew },
		{ "import", limport },
		{ "byte", lpackbyte },
		{ "word", lpackword },
		{ "int32", lpackint32 },
		{ "color", lpackcolor },
		{ "string", lpackstring },
		{ "frametag", lpackframetag },
		{ "picture_size", lpicture_size },
		{ "polygon_size", lpolygon_size },
		{ "pack_size", lpack_size },
		{ "animation_size", lanimation_size },
		{ "part_size", lpart_size },
		{ "string_size" , lstring_size },
		{ "label_size", llabel_size },
		{ "panel_size", lpanel_size },
		{0, 0},
	};
	luaL_newlib(L, l);
	lua_pushinteger(L, TYPE_PICTURE);
	lua_setfield(L, -2, "TYPE_PICTURE");
	lua_pushinteger(L, TYPE_ANIMATION);
	lua_setfield(L, -2, "TYPE_ANIMATION");
	lua_pushinteger(L, TYPE_POLYGON);
	lua_setfield(L, -2, "TYPE_POLYGON");
	lua_pushinteger(L, TYPE_LABEL);
	lua_setfield(L, -2, "TYPE_LABEL");
	lua_pushinteger(L, TYPE_PANEL);
	lua_setfield(L, -2, "TYPE_PANEL");
	lua_pushinteger(L, TYPE_MATRIX);
	lua_setfield(L, -2, "TYPE_MATRIX");
	return 1;
}

#endif // PIXEL_LUA
