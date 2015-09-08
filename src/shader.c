#include "shader.h"
#include "render.h"
#include "renderbuffer.h"
#include "texture.h"
#include "screen.h"
#include "material.h"
#include "blend.h"
#include "pixel.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>

#define MAX_PROGRAM 16
#define MAX_UNIFORM 16
#define MAX_TEXTURE_CHANNEL 8

#define BUFFER_OFFSET(f) ((intptr_t)&(((struct vertex *)NULL)->f))

struct uniform {
	int loc;
	int offset;
	enum UNIFORM_FORMAT type;
};

struct program {
	int pid;
	struct material *material;
	int texture_n;
	int uniform_n;
	struct uniform uniform[MAX_UNIFORM];
	int reset_uniform;
	int uniform_change[MAX_UNIFORM];
	float uniform_val[MAX_UNIFORM * 16];
};

struct shader {
	int cur_pid;
	struct program p[MAX_PROGRAM];
	int tid[MAX_TEXTURE_CHANNEL];
	struct renderbuffer rb;
	int blendchange;
	int vertex_buffer;
	int index_buffer;
	int layout;
};

static struct shader S;

int shader_version(void) {
	return render_version();
}

void shader_init(void) {
	int i;
	struct render_arg arg;
	uint16_t idxs[6 * MAX_COMMBINE];
	struct vertex_attrib va[4] = {
		{ "position", 0, 2, sizeof(float), BUFFER_OFFSET(vp.vx) },
		{ "texcoord", 0, 2, sizeof(uint16_t), BUFFER_OFFSET(vp.tx) },
		{ "color", 0, 4, sizeof(uint8_t), BUFFER_OFFSET(rgba) },
		{ "additive", 0, 4, sizeof(uint8_t), BUFFER_OFFSET(addi) },
	};

	arg.max_buffer = 128;
	arg.max_layout = 4;
	arg.max_target = 128;
	arg.max_texture = 256;
	arg.max_shader = MAX_PROGRAM;

	render_init(&arg);
	S.cur_pid = -1;
	S.blendchange = 0;
	render_setblend(BLEND_ONE, BLEND_ONE_MINUS_SRC_ALPHA);

	for (i = 0; i < MAX_COMMBINE; i++) {
		idxs[i * 6] = i * 4;
		idxs[i * 6 + 1] = i * 4 + 1;
		idxs[i * 6 + 2] = i * 4 + 2;
		idxs[i * 6 + 3] = i * 4;
		idxs[i * 6 + 4] = i * 4 + 2;
		idxs[i * 6 + 5] = i * 4 + 3;
	}
	S.index_buffer = render_buffer_create(INDEXBUFFER, idxs, 6 * MAX_COMMBINE, sizeof(uint16_t));
	S.vertex_buffer = render_buffer_create(VERTEXBUFFER, 0, 4 * MAX_COMMBINE, sizeof(struct vertex));
	S.layout = render_vertexlayout(sizeof(va) / sizeof(va[0]), va);
	render_set(VERTEXLAYOUT, S.layout, 0);
	render_set(INDEXBUFFER, S.index_buffer, 0);
	render_set(VERTEXBUFFER, S.vertex_buffer, 0);
}

void shader_unit(void) {
	render_unit();
}

void shader_load(int pid, const char *fragment, const char *vertex, int texture, const char **texture_uniform) {
	struct program *p;
	struct shader_arg arg;

	assert(pid >= 0 && pid < MAX_PROGRAM);
	p = &S.p[pid];
	if (p->pid) {
		render_rem(SHADER, p->pid);
		p->pid = 0;
	}
	if (!fragment || !vertex) {
		pixel_log("shader load %s %s failed\n", fragment, vertex);
		return;
	}
	memset(p, 0, sizeof(*p));
	arg.vs = vertex;
	arg.fs = fragment;
	arg.texture = texture;
	arg.texture_uniform = texture_uniform;
	p->pid = render_shader_create(&arg);
	render_shader_bind(p->pid);
	p->texture_n = texture;
	S.cur_pid = -1;
}

void shader_flush(void) {
	struct renderbuffer *rb = &S.rb;
	if (rb->object == 0) {
		return;
	}
	render_buffer_update(S.vertex_buffer, rb->vb, 4 * rb->object);
	render_draw(DRAW_TRIANGLE, 0, 6 * rb->object);
	rb->object = 0;
}

void shader_texture(int id, int channel) {
	assert(channel < MAX_TEXTURE_CHANNEL);
	if (S.tid[channel] != id) {
		shader_flush();
		S.tid[channel] = id;
		render_set(TEXTURE, id, channel);
	}
}

static void set_uniform(struct program *p) {
	int i;
	for (i = 0; i < p->uniform_n; i++) {
		if (p->uniform_change[i]) {
			struct uniform *u = &p->uniform[i];
			if (u->loc >= 0) {
				render_shader_uniform(u->loc, u->type, p->uniform_val + u->offset);
			}
		}
	}
	p->reset_uniform = 0;
}

void shader_program(int pid, struct material *m) {
	struct program *p = &S.p[pid];
	if (S.cur_pid != pid || p->reset_uniform || m) {
		shader_flush();
	}
	if (S.cur_pid != pid) {
		S.cur_pid = pid;
		render_shader_bind(p->pid);
		p->material = 0;
		set_uniform(p);
	} else if (p->reset_uniform) {
		set_uniform(p);
	}
	if (m) {
		material_apply(m, pid);
	}
}

void shader_clear(unsigned long argb) {
	render_clear(MASKC, argb);
}

void shader_blend(int m1, int m2) {
	if (m1 != BLEND_ONE || m2 != BLEND_GL_ONE_MINUS_SRC_ALPHA) {
		shader_flush();
		S.blendchange = 1;
		render_setblend(blend_mode(m1), blend_mode(m2));
	}
}

void shader_default_blend(void) {
	if (S.blendchange) {
		shader_flush();
		S.blendchange = 0;
		render_setblend(BLEND_ONE, BLEND_ONE_MINUS_SRC_ALPHA);
	}
}

void shader_scissor(int enable) {
	render_enable_scissor(enable);
}

int shader_add_uniform(int pid, const char *name, enum UNIFORM_FORMAT t) {
	struct program *p;
	struct uniform *u;
	int loc, idx;

	assert(pid >= 0 && pid < MAX_PROGRAM);
	shader_program(pid, 0);
	p = &S.p[pid];
	assert(p->uniform_n < MAX_UNIFORM);
	loc = render_shader_locuniform(name);
	idx = p->uniform_n++;
	u = &p->uniform[idx];
	u->loc = loc;
	u->type = t;
	if (idx == 0) {
		u->offset = 0;
	} else {
		struct uniform *lu = &p->uniform[idx - 1];
		u->offset = lu->offset + shader_uniform_size(lu->type);
	}
	if (loc < 0) {
		return -1;
	}
	return idx;
}

void shader_set_uniform(int pid, int idx, enum UNIFORM_FORMAT t, float *v) {
	struct program *p;
	struct uniform *u;
	int n;

	shader_flush();
	p = &S.p[pid];
	assert(idx >= 0 && idx < p->uniform_n);
	u = &p->uniform[idx];
	assert(t == u->type);
	n = shader_uniform_size(t);
	memcpy(p->uniform_val + u->offset, v, n*sizeof(float));
	p->reset_uniform = 1;
	p->uniform_change[idx] = 1;
}

int shader_uniform_size(enum UNIFORM_FORMAT t) {
	int n = 0;
	switch (t) {
	case UNIFORM_INVALID:
		n = 0;
		break;
	case UNIFORM_FLOAT1:
		n = 1;
		break;
	case UNIFORM_FLOAT2:
		n = 2;
		break;
	case UNIFORM_FLOAT3:
		n = 3;
		break;
	case UNIFORM_FLOAT4:
		n = 4;
		break;
	case UNIFORM_FLOAT33:
		n = 9;
		break;
	case UNIFORM_FLOAT44:
		n = 16;
		break;
	}
	return n;
}

void shader_drawvertex(const struct vertex_pack vp[4], uint32_t color, uint32_t addi) {
	if (renderbuffer_addvertex(&S.rb, vp, color, addi)) {
		shader_flush();
	}
}

static void shader_drawquad(const struct vertex_pack *vp, uint32_t color, uint32_t addi, int idx, int max) {
	struct vertex_pack _vp[4];
	int i;

	_vp[0] = vp[0];
	for (i = 1; i < 4; i++) {
		int j = i + idx;
		int n = j <= max ? j : max;
		_vp[i] = vp[n];
	}
	shader_drawvertex(_vp, color, addi);
}

void shader_drawpolygon(int n, const struct vertex_pack *vp, uint32_t color, uint32_t addi) {
	int i = 0;
	n--;
	do {
		shader_drawquad(vp, color, addi, i, n);
		i += 2;
	} while (i < n - 1);
}

void shader_drawbuffer(struct renderbuffer *rb, float tx, float ty, float scale) {
	int rid;
	float sx, sy;
	float v[4];

	shader_flush();
	rid = texture_rid(rb->tid);
	if (rid == 0) {
		return;
	}
	shader_texture(rid, 0);
	render_set(VERTEXBUFFER, rb->bid, 0);

	sx = scale;
	sy = scale;
	screen_trans(&sx, &sy);
	screen_trans(&tx, &ty);

	v[0] = sx;
	v[1] = sy;
	v[2] = tx;
	v[3] = ty;
	shader_set_uniform(PROGRAM_RENDERBUFFER, 0, UNIFORM_FLOAT4, v);
	shader_program(PROGRAM_RENDERBUFFER, 0);
	render_draw(DRAW_TRIANGLE, 0, 6 * rb->object);
	render_set(VERTEXBUFFER, S.vertex_buffer, 0);
}

void shader_draw(int tid, const float tcoord[8], const float scoord[8], uint32_t color, uint32_t addi) {
	int i;
	struct vertex_pack vp[4];

	for (i = 0; i < 4; i++) {
		uint16_t u, v;
		float tx = tcoord[i * 2 + 0];
		float ty = tcoord[i * 2 + 1];
		float vx = scoord[i * 2 + 0];
		float vy = scoord[i * 2 + 1];
		screen_trans(&vx, &vy);
		texture_coord(tid, tx, ty, &u, &v);
		vp[i].vx = vx + 1.0f;
		vp[i].vy = vy - 1.0f;
		vp[i].tx = u;
		vp[i].ty = v;
	}
	shader_program(PROGRAM_PICTURE, 0);
	shader_texture(texture_rid(tid), 0);
	shader_drawvertex(vp, color, addi);
}

struct material {
	struct program *p;
	int texture[MAX_TEXTURE_CHANNEL];
	int uniform_enable[MAX_UNIFORM];
	float uniform[1];
};

int material_size(int pid) {
	struct program *p;
	struct uniform *lu;
	int total;

	if (pid < 0 || pid >= MAX_PROGRAM) {
		return 0;
	}
	p = &S.p[pid];
	if (p->uniform_n == 0 && p->texture_n == 0) {
		return 0;
	}
	lu = &p->uniform[p->uniform_n - 1];
	total = lu->offset + shader_uniform_size(lu->type);
	return sizeof(struct material) + (total - 1)*sizeof(float);
}

struct material *material_init(struct material *m, int pid) {
	struct program *p;
	int i;

	p = &S.p[pid];
	m->p = p;
	for (i = 0; i < MAX_TEXTURE_CHANNEL; i++) {
		m->texture[i] = -1;
	}
	return m;
}

int material_uniform(struct material *m, int idx, int n, const float *v) {
	struct program *p;
	struct uniform *u;

	p = m->p;
	assert(idx >= 0 && idx < p->uniform_n);
	p = m->p;
	u = &p->uniform[idx];
	if (shader_uniform_size(u->type) != n) {
		return -1;
	}
	memcpy(m->uniform + u->offset, v, n*sizeof(float));
	m->uniform_enable[idx] = 1;
	return 0;
}

int material_texture(struct material *m, int channel, int tid) {
	if (channel >= MAX_TEXTURE_CHANNEL) {
		return -1;
	}
	m->texture[channel] = tid;
	return 0;
}

void material_apply(struct material *m, int pid) {
	int i;
	struct program *p = m->p;
	if (p != &S.p[pid]) {
		return;
	}
	if (p->material == m) {
		return;
	}
	p->material = m;
	p->reset_uniform = 1;
	for (i = 0; i < p->uniform_n; i++) {
		if (m->uniform_enable[i]) {
			struct uniform *u = &p->uniform[i];
			if (u->loc >= 0) {
				render_shader_uniform(u->loc, u->type, m->uniform + u->offset);
			}
		}
	}
	for (i = 0; i < p->texture_n; i++) {
		int tid = m->texture[i];
		if (tid >= 0) {
			int rid = texture_rid(tid);
			if (rid) {
				shader_texture(rid, i);
			}
		}
	}
}


#ifdef PIXEL_LUA

#include "lua.h"
#include "lauxlib.h"

static int lversion(lua_State *L) {
	lua_pushinteger(L, shader_version());
	return 1;
}

static int lload(lua_State *L) {
	int pid = (int)luaL_checkinteger(L, 1);
	const char *fs = luaL_checkstring(L, 2);
	const char *vs = luaL_checkstring(L, 3);
	if (lua_istable(L, 4)) {
		int i;
		int texture_n = lua_rawlen(L, 4);
#if defined(_MSC_VER)
		const char **name = _alloca(texture_n * sizeof(const char *));
#else
		const char *name[texture_n];
#endif
		luaL_checkstack(L, texture_n + 1, 0);
		for (i = 0; i < texture_n; i++) {
			lua_rawgeti(L, -1 - i, i + 1);
			name[i] = luaL_checkstring(L, -1);
		}
		shader_load(pid, fs, vs, texture_n, name);
	} else {
		shader_load(pid, fs, vs, 0, 0);
	}
	return 0;
}

static int ldraw(lua_State *L) {
	int n, np;
	uint32_t additive = 0;
	uint32_t color = 0xffffffff;
	int tid = (int)luaL_checkinteger(L, 1);
	int rid = texture_rid(tid);
	if (rid <= 0) {
		lua_pushboolean(L, 0);
		return 1;
	}
	luaL_checktype(L, 2, LUA_TTABLE);
	luaL_checktype(L, 3, LUA_TTABLE);
	if (!lua_isnoneornil(L, 4)) {
		color = (uint32_t)lua_tointeger(L, 4);
	}
	additive = (uint32_t)luaL_optinteger(L, 5, 0);
	shader_program(PROGRAM_PICTURE, 0);
	shader_texture(rid, 0);
	n = lua_rawlen(L, 2);
	np = n / 2;
	if (lua_rawlen(L, 3) != n || n % 2 != 0) {
		return luaL_error(L, "shader.draw invalid texture or screen");
	} else {
		int i;
#if defined(_MSC_VER)
		struct vertex_pack *vp = _alloca(np * sizeof(struct vertex_pack));
#else
		struct vertex_pack vp[np];
#endif
		for (i = 0; i < np; i++) {
			float tx, ty;
			lua_rawgeti(L, 2, i * 2 + 1);
			lua_rawgeti(L, 2, i * 2 + 2);
			tx = (float)lua_tonumber(L, -2);
			ty = (float)lua_tonumber(L, -1);
			lua_pop(L, 2);
			texture_coord(tid, tx, ty, &vp[i].tx, &vp[i].ty);
		}
		for (i = 0; i < np; i++) {
			float vx, vy;
			lua_rawgeti(L, 3, i * 2 + 1);
			lua_rawgeti(L, 3, i * 2 + 2);
			vx = (float)lua_tonumber(L, -2);
			vy = (float)lua_tonumber(L, -1);
			lua_pop(L, 2);
			screen_trans(&vx, &vy);
			vp[i].vx = vx + 1.0f;
			vp[i].vy = vy - 1.0f;
		}
		if (np == 4) {
			shader_drawvertex(vp, color, additive);
		} else {
			shader_drawpolygon(np, vp, color, additive);
		}
	}
	return 0;
}

static int lclear(lua_State *L) {
	uint32_t color = (uint32_t)luaL_optinteger(L, 1, 0xff000000);
	shader_clear(color);
	return 0;
}

static int lblend(lua_State *L) {
	if (lua_isnoneornil(L, 1)) {
		shader_default_blend();
	} else {
		int m1 = (int)luaL_checkinteger(L, 1);
		int m2 = (int)luaL_checkinteger(L, 2);
		shader_blend(m1, m2);
	}
	return 0;
}

static int luniform_set(lua_State *L) {
	int i;
	float v[16];
	int pid = (int)luaL_checkinteger(L, 1);
	int idx = (int)luaL_checkinteger(L, 2);
	enum UNIFORM_FORMAT t = (enum UNIFORM_FORMAT)luaL_checkinteger(L, 3);
	int n = shader_uniform_size(t);
	if (n == 0) {
		return luaL_error(L, "invalid uniform format %d", t);
	}
	for (i = 0; i < n; i++) {
		v[i] = (float)luaL_checknumber(L, 4 + i);
	}
	shader_set_uniform(pid, idx, t, v);
	return 0;
}

static int luniform_bind(lua_State *L) {
	int i, n;
	int pid = (int)luaL_checkinteger(L, 1);
	luaL_checktype(L, 2, LUA_TTABLE);
	n = lua_rawlen(L, 2);
	for (i = 0; i < n; i++) {
		int loc;
		const char *name;
		enum UNIFORM_FORMAT t;
		lua_rawgeti(L, -1, i + 1);
		lua_getfield(L, -1, "name");
		name = luaL_checkstring(L, -1);
		lua_getfield(L, -2, "type");
		t = (enum UNIFORM_FORMAT)luaL_checkinteger(L, -1);
		loc = shader_add_uniform(pid, name, t);
		if (loc != i) {
			return luaL_error(L, "invalid uniform loc %s", name);
		}
		lua_pop(L, 3);
	}
	return 0;
}

static int ltexture(lua_State *L) {
	int channel;
	int rid = 0;
	if (!lua_isnoneornil(L, 1)) {
		int tid = (int)luaL_checkinteger(L, 1);
		rid = texture_rid(tid);
	}
	channel = (int)luaL_optinteger(L, 2, 0);
	shader_texture(rid, channel);
	return 0;
}

static int lmaterial_uniform(lua_State *L) {
	int i, n;
	float v[16];
	struct material *m = (struct material *)lua_touserdata(L, 1);
	int idx = (int)luaL_checkinteger(L, 2);
	n = lua_gettop(L) - 2;
	for (i = 0; i < n; i++) {
		v[i] = (float)luaL_checknumber(L, i + 3);
	}
	if (material_uniform(m, idx, n, v)) {
		return luaL_error(L, "invalid argument %d", n);
	}
	return 0;
}

static int lmaterial_texture(lua_State *L) {
	struct material *m = (struct material *)lua_touserdata(L, 1);
	int tid = (int)luaL_checkinteger(L, 2);
	int channel = (int)luaL_optinteger(L, 3, 0);
	if (material_texture(m, channel, tid)) {
		return luaL_error(L, "invalid texture channel %d", channel);
	}
	return 0;
}

int pixel_shader(lua_State *L) {
	luaL_Reg l[] = {
		{"version", lversion},
		{"load", lload},
		{"draw", ldraw},
		{"clear", lclear},
		{"blend", lblend},
		{"texture", ltexture},
		{"uniform_set", luniform_set},
		{"uniform_bind", luniform_bind},
		{"material_uniform", lmaterial_uniform},
		{"material_texture", lmaterial_texture},
		{0, 0},
	};
	luaL_newlib(L, l);
	return 1;
}

#endif // PIXEL_LUA