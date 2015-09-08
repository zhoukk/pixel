#include "matrix.h"
#include <stdint.h>

static inline int _inverse_scale(const int *m, int *o) {
	if (m[0] == 0 || m[3] == 0)
		return 1;
	o[0] = (1024 * 1024) / m[0];
	o[1] = 0;
	o[2] = 0;
	o[3] = (1024 * 1024) / m[3];
	o[4] = -(m[4] * o[0]) / 1024;
	o[5] = -(m[5] * o[3]) / 1024;
	return 0;
}

static inline int _inverse_rot(const int *m, int *o) {
	if (m[1] == 0 || m[2] == 0)
		return 1;
	o[0] = 0;
	o[1] = (1024 * 1024) / m[2];
	o[2] = (1024 * 1024) / m[1];
	o[3] = 0;
	o[4] = -(m[5] * o[2]) / 1024;
	o[5] = -(m[4] * o[1]) / 1024;
	return 0;
}

int matrix_inverse(const struct matrix *mm, struct matrix *mo) {
	int t;
	const int *m = mm->m;
	int *o = mo->m;
	if (m[1] == 0 && m[2] == 0) {
		return _inverse_scale(m, o);
	}
	if (m[0] == 0 && m[3] == 0) {
		return _inverse_rot(m, o);
	}
	t = m[0] * m[3] - m[1] * m[2];
	if (t == 0)
		return 1;
	o[0] = (int32_t)((int64_t)m[3] * (1024 * 1024) / t);
	o[1] = (int32_t)(-(int64_t)m[1] * (1024 * 1024) / t);
	o[2] = (int32_t)(-(int64_t)m[2] * (1024 * 1024) / t);
	o[3] = (int32_t)((int64_t)m[0] * (1024 * 1024) / t);
	o[4] = -(m[4] * o[0] + m[5] * o[2]) / 1024;
	o[5] = -(m[4] * o[1] + m[5] * o[3]) / 1024;
	return 0;
}

// SRT to matrix

static inline int icost(int dd) {
	static int t[256] = {
		1024,1023,1022,1021,1019,1016,1012,1008,1004,999,993,986,979,972,964,955,
		946,936,925,914,903,890,878,865,851,837,822,807,791,775,758,741,
		724,706,687,668,649,629,609,589,568,547,526,504,482,460,437,414,
		391,368,344,321,297,273,248,224,199,175,150,125,100,75,50,25,
		0,-25,-50,-75,-100,-125,-150,-175,-199,-224,-248,-273,-297,-321,-344,-368,
		-391,-414,-437,-460,-482,-504,-526,-547,-568,-589,-609,-629,-649,-668,-687,-706,
		-724,-741,-758,-775,-791,-807,-822,-837,-851,-865,-878,-890,-903,-914,-925,-936,
		-946,-955,-964,-972,-979,-986,-993,-999,-1004,-1008,-1012,-1016,-1019,-1021,-1022,-1023,
		-1024,-1023,-1022,-1021,-1019,-1016,-1012,-1008,-1004,-999,-993,-986,-979,-972,-964,-955,
		-946,-936,-925,-914,-903,-890,-878,-865,-851,-837,-822,-807,-791,-775,-758,-741,
		-724,-706,-687,-668,-649,-629,-609,-589,-568,-547,-526,-504,-482,-460,-437,-414,
		-391,-368,-344,-321,-297,-273,-248,-224,-199,-175,-150,-125,-100,-75,-50,-25,
		0,25,50,75,100,125,150,175,199,224,248,273,297,321,344,368,
		391,414,437,460,482,504,526,547,568,589,609,629,649,668,687,706,
		724,741,758,775,791,807,822,837,851,865,878,890,903,914,925,936,
		946,955,964,972,979,986,993,999,1004,1008,1012,1016,1019,1021,1022,1023,
	};
	dd = dd & 0xff;
	return t[dd];
}

static inline int icosd(int d) {
	int dd = d / 4;
	return icost(dd);
}

static inline int isind(int d) {
	int dd = 64 - d / 4;
	return icost(dd);
}

static inline void rot_mat(int *m, int d) {
	int cosd = icosd(d);
	int sind = isind(d);

	int m0_cosd = m[0] * cosd;
	int m0_sind = m[0] * sind;
	int m1_cosd = m[1] * cosd;
	int m1_sind = m[1] * sind;
	int m2_cosd = m[2] * cosd;
	int m2_sind = m[2] * sind;
	int m3_cosd = m[3] * cosd;
	int m3_sind = m[3] * sind;
	int m4_cosd = m[4] * cosd;
	int m4_sind = m[4] * sind;
	int m5_cosd = m[5] * cosd;
	int m5_sind = m[5] * sind;

	m[0] = (m0_cosd - m1_sind) / 1024;
	m[1] = (m0_sind + m1_cosd) / 1024;
	m[2] = (m2_cosd - m3_sind) / 1024;
	m[3] = (m2_sind + m3_cosd) / 1024;
	m[4] = (m4_cosd - m5_sind) / 1024;
	m[5] = (m4_sind + m5_cosd) / 1024;
}

static inline void scale_mat(int *m, int sx, int sy) {
	if (sx != 1024) {
		m[0] = m[0] * sx / 1024;
		m[2] = m[2] * sx / 1024;
		m[4] = m[4] * sx / 1024;
	}
	if (sy != 1024) {
		m[1] = m[1] * sy / 1024;
		m[3] = m[3] * sy / 1024;
		m[5] = m[5] * sy / 1024;
	}
}

void matrix_srt(struct matrix *mm, const struct srt *srt) {
	if (!srt) {
		return;
	}
	scale_mat(mm->m, srt->scalex, srt->scaley);
	rot_mat(mm->m, srt->rot);
	mm->m[4] += srt->offx;
	mm->m[5] += srt->offy;
}

void matrix_rot(struct matrix *m, int rot) {
	rot_mat(m->m, rot);
}

void matrix_sr(struct matrix *mat, int sx, int sy, int d) {
	int *m = mat->m;
	int cosd = icosd(d);
	int sind = isind(d);

	int m0_cosd = sx * cosd;
	int m0_sind = sx * sind;
	int m3_cosd = sy * cosd;
	int m3_sind = sy * sind;

	m[0] = m0_cosd / 1024;
	m[1] = m0_sind / 1024;
	m[2] = -m3_sind / 1024;
	m[3] = m3_cosd / 1024;
}

void matrix_rs(struct matrix *mat, int sx, int sy, int d) {
	int *m = mat->m;
	int cosd = icosd(d);
	int sind = isind(d);

	int m0_cosd = sx * cosd;
	int m0_sind = sx * sind;
	int m3_cosd = sy * cosd;
	int m3_sind = sy * sind;

	m[0] = m0_cosd / 1024;
	m[1] = m3_sind / 1024;
	m[2] = -m0_sind / 1024;
	m[3] = m3_cosd / 1024;
}

void matrix_scale(struct matrix *m, int sx, int sy) {
	scale_mat(m->m, sx, sy);
}


#ifdef PIXEL_LUA

#include "lua.h"
#include "lauxlib.h"
#include "screen.h"

#include <string.h>

static int lscale(lua_State *L) {
	struct matrix *mat = (struct matrix *)lua_touserdata(L, 1);
	double sx = luaL_checknumber(L, 2);
	double sy = luaL_optnumber(L, 3, sx);
	matrix_scale(mat, (int)(sx * 1024), (int)(sy * 1024));
	lua_settop(L, 1);
	return 1;
}

static int lrot(lua_State *L) {
	struct matrix *mat = (struct matrix *)lua_touserdata(L, 1);
	double rot = luaL_checknumber(L, 2);
	matrix_rot(mat, (int)(rot*(1024.0 / 360.0)));
	lua_settop(L, 1);
	return 1;
}

static int lsr(lua_State *L) {
	struct matrix *mat = (struct matrix *)lua_touserdata(L, 1);
	int sx = 1024, sy = 1024, r = 0;
	switch (lua_gettop(L)) {
	case 4:
		r = (int)(luaL_checknumber(L, 4)*(1024.0 / 360.0));
	case 3:
		sx = (int)(luaL_checknumber(L, 2) * 1024);
		sy = (int)(luaL_checknumber(L, 3) * 1024);
		break;
	case 2:
		r = (int)(luaL_checknumber(L, 4)*(1024.0 / 360.0));
		break;
	}
	matrix_sr(mat, sx, sy, r);
	return 0;
}

static int lrs(lua_State *L) {
	struct matrix *mat = (struct matrix *)lua_touserdata(L, 1);
	int sx = 1024, sy = 1024, r = 0;
	switch (lua_gettop(L)) {
	case 4:
		r = (int)(luaL_checknumber(L, 4)*(1024.0 / 360.0));
	case 3:
		sx = (int)(luaL_checknumber(L, 2) * 1024);
		sy = (int)(luaL_checknumber(L, 3) * 1024);
		break;
	case 2:
		r = (int)(luaL_checknumber(L, 2)*(1024.0 / 360.0));
		break;
	}
	matrix_rs(mat, sx, sy, r);
	return 0;
}

static int ltrans(lua_State *L) {
	struct matrix *mat = (struct matrix *)lua_touserdata(L, 1);
	double x = luaL_checknumber(L, 2);
	double y = luaL_checknumber(L, 3);
	mat->m[4] += (int)(x * SCREEN_SCALE);
	mat->m[5] += (int)(y * SCREEN_SCALE);
	lua_settop(L, 1);
	return 1;
}

static int lmul(lua_State *L) {
	struct matrix *mat1 = (struct matrix *)lua_touserdata(L, 1);
	struct matrix *mat2 = (struct matrix *)lua_touserdata(L, 2);
	struct matrix tmp = *mat1;
	matrix_mul(mat1, &tmp, mat2);
	lua_settop(L, 1);
	return 1;
}

static int llmul(lua_State *L) {
	struct matrix *mat1 = (struct matrix *)lua_touserdata(L, 1);
	struct matrix *mat2 = (struct matrix *)lua_touserdata(L, 2);
	struct matrix tmp = *mat1;
	matrix_mul(mat1, mat2, &tmp);
	lua_settop(L, 1);
	return 1;
}

static int limport(lua_State *L) {
	struct matrix *mat = (struct matrix *)lua_touserdata(L, 1);
	int *m = mat->m;
	int i;
	for (i = 0; i < 6; i++) {
		m[i] = (int)luaL_checkinteger(L, i + 2);
	}
	return 0;
}

static int lexport(lua_State *L) {
	struct matrix *mat = (struct matrix *)lua_touserdata(L, 1);
	int *m = mat->m;
	int i;
	for (i = 0; i < 6; i++) {
		lua_pushinteger(L, m[i]);
	}
	return 6;
}

static int ltostring(lua_State *L) {
	struct matrix *mat = (struct matrix *)lua_touserdata(L, 1);
	int *m = mat->m;
	lua_pushfstring(L, "{%d,%d,%d,%d,%d,%d}", m[0], m[1], m[2], m[3], m[4], m[5]);
	return 1;
}

static int lnew(lua_State *L) {
	struct matrix *mat;
	int *m;

	lua_settop(L, 1);
	mat = (struct matrix *)lua_newuserdata(L, sizeof *mat);
	m = mat->m;
	if (lua_istable(L, 1) && lua_rawlen(L, 1) == 6) {
		int i;
		for (i = 0; i < 6; i++) {
			lua_rawgeti(L, 1, i + 1);
			m[i] = (int)lua_tointeger(L, -1);
			lua_pop(L, 1);
		}
	} else if (lua_isuserdata(L, 1)) {
		memcpy(m, lua_touserdata(L, 1), 6 * sizeof(int));
	} else {
		m[0] = 1024;
		m[1] = 0;
		m[2] = 0;
		m[3] = 1024;
		m[4] = 0;
		m[5] = 0;
	}
	if (luaL_newmetatable(L, "matrix")) {
		luaL_Reg l[] = {
			{"scale", lscale},
			{"rot", lrot},
			{"sr", lsr},
			{"rs", lrs},
			{"trans", ltrans},
			{"mul", lmul},
			{"lmul", llmul},
			{"import", limport},
			{"export", lexport},
			{0, 0},
		};
		luaL_newlib(L, l);
		lua_setfield(L, -2, "__index");
		lua_pushcfunction(L, ltostring);
		lua_setfield(L, -2, "__tostring");
	}
	lua_setmetatable(L, -2);
	return 1;
}

int pixel_matrix(lua_State *L) {
	luaL_Reg l[] = {
		{"new", lnew},
		{0, 0},
	};
	luaL_newlib(L, l);
	return 1;
}

#endif // PIXEL_LUA