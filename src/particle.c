#include "particle.h"
#include "sprite.h"
#include "matrix.h"
#include "screen.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#define CC_DEGREES_TO_RADIANS(__ANGLE__) ((__ANGLE__) * 0.01745329252f) // PI / 180
#define CC_RADIANS_TO_DEGREES(__ANGLE__) ((__ANGLE__) * 57.29577951f) // PI * 180

#define PARTICLE_MODE_GRAVITY 0
#define PARTICLE_MODE_RADIUS 1

#define POSITION_TYPE_FREE 0
#define POSITION_TYPE_RELATIVE 1
#define POSITION_TYPE_GROUPED 2

/** The Particle emitter lives forever */
#define DURATION_INFINITY (-1)

/** The starting size of the particle is equal to the ending size */
#define START_SIZE_EQUAL_TO_END_SIZE (-1)

/** The starting radius of the particle is equal to the ending radius */
#define START_RADIUS_EQUAL_TO_END_RADIUS (-1)

static inline float clampf(float x) {
	if (x < 0)
		return 0;
	if (x > 1.0f)
		return 1.0f;
	return x;
}

inline static float RANDOM_M11(unsigned int *seed) {
	union {
		uint32_t d;
		float f;
	} u;
	*seed = *seed * 134775813 + 1;
	u.d = (((uint32_t)(*seed) & 0x7fff) << 8) | 0x40000000;
	return u.f - 3.0f;
}

static void _initParticle(struct particle_system *ps, struct particle* particle) {
	uint32_t RANDSEED = rand();
	int *mat;
	struct color4f *start;
	struct color4f end;
	float startS;
	float startA, endA, a;

	particle->timeToLive = ps->config->life + ps->config->lifeVar * RANDOM_M11(&RANDSEED);
	if (particle->timeToLive <= 0) {
		return;
	}
	mat = particle->emitMatrix.m;
	if (ps->config->emitterMatrix) {
		memcpy(mat, ps->config->emitterMatrix->m, 6 * sizeof(int));
	} else {
		mat[0] = 1024;
		mat[1] = 0;
		mat[2] = 0;
		mat[3] = 1024;
		mat[4] = 0;
		mat[5] = 0;
	}
	particle->startPos = ps->config->sourcePosition;
	particle->pos.x = ps->config->posVar.x * RANDOM_M11(&RANDSEED);
	particle->pos.y = ps->config->posVar.y * RANDOM_M11(&RANDSEED);

	start = &particle->color;
	start->r = clampf(ps->config->startColor.r + ps->config->startColorVar.r * RANDOM_M11(&RANDSEED));
	start->g = clampf(ps->config->startColor.g + ps->config->startColorVar.g * RANDOM_M11(&RANDSEED));
	start->b = clampf(ps->config->startColor.b + ps->config->startColorVar.b * RANDOM_M11(&RANDSEED));
	start->a = clampf(ps->config->startColor.a + ps->config->startColorVar.a * RANDOM_M11(&RANDSEED));

	end.r = clampf(ps->config->endColor.r + ps->config->endColorVar.r * RANDOM_M11(&RANDSEED));
	end.g = clampf(ps->config->endColor.g + ps->config->endColorVar.g * RANDOM_M11(&RANDSEED));
	end.b = clampf(ps->config->endColor.b + ps->config->endColorVar.b * RANDOM_M11(&RANDSEED));
	end.a = clampf(ps->config->endColor.a + ps->config->endColorVar.a * RANDOM_M11(&RANDSEED));

	particle->deltaColor.r = (end.r - start->r) / particle->timeToLive;
	particle->deltaColor.g = (end.g - start->g) / particle->timeToLive;
	particle->deltaColor.b = (end.b - start->b) / particle->timeToLive;
	particle->deltaColor.a = (end.a - start->a) / particle->timeToLive;

	startS = ps->config->startSize + ps->config->startSizeVar * RANDOM_M11(&RANDSEED);
	if (startS < 0) {
		startS = 0;
	}
	particle->size = startS;
	if (ps->config->endSize == START_SIZE_EQUAL_TO_END_SIZE) {
		particle->deltaSize = 0;
	} else {
		float endS = ps->config->endSize + ps->config->endSizeVar * RANDOM_M11(&RANDSEED);
		if (endS < 0) {
			endS = 0;
		}
		particle->deltaSize = (endS - startS) / particle->timeToLive;
	}
	// rotation
	startA = ps->config->startSpin + ps->config->startSpinVar * RANDOM_M11(&RANDSEED);
	endA = ps->config->endSpin + ps->config->endSpinVar * RANDOM_M11(&RANDSEED);
	particle->rotation = startA;
	particle->deltaRotation = (endA - startA) / particle->timeToLive;

	// direction
	a = CC_DEGREES_TO_RADIANS(ps->config->angle + ps->config->angleVar * RANDOM_M11(&RANDSEED));

	// Mode Gravity: A
	if (ps->config->emitterMode == PARTICLE_MODE_GRAVITY) {
		struct point v;
		float s;
		v.x = cosf(a);
		v.y = sinf(a);
		s = ps->config->mode.A.speed + ps->config->mode.A.speedVar * RANDOM_M11(&RANDSEED);

		// direction
		particle->mode.A.dir.x = v.x * s;
		particle->mode.A.dir.y = v.y * s;

		// radial accel
		particle->mode.A.radialAccel = ps->config->mode.A.radialAccel + ps->config->mode.A.radialAccelVar * RANDOM_M11(&RANDSEED);

		// tangential accel
		particle->mode.A.tangentialAccel = ps->config->mode.A.tangentialAccel + ps->config->mode.A.tangentialAccelVar * RANDOM_M11(&RANDSEED);

		// rotation is dir
		if (ps->config->mode.A.rotationIsDir) {
			struct point *p = &(particle->mode.A.dir);
			particle->rotation = -CC_RADIANS_TO_DEGREES(atan2f(p->y, p->x));
		}
	}
	// Mode Radius: B
	else {
		// Set the default diameter of the particle from the source position
		float startRadius = ps->config->mode.B.startRadius + ps->config->mode.B.startRadiusVar * RANDOM_M11(&RANDSEED);
		float endRadius = ps->config->mode.B.endRadius + ps->config->mode.B.endRadiusVar * RANDOM_M11(&RANDSEED);

		particle->mode.B.radius = startRadius;

		if (ps->config->mode.B.endRadius == START_RADIUS_EQUAL_TO_END_RADIUS) {
			particle->mode.B.deltaRadius = 0;
		} else {
			particle->mode.B.deltaRadius = (endRadius - startRadius) / particle->timeToLive;
		}
		particle->mode.B.angle = a;
		particle->mode.B.degreesPerSecond = CC_DEGREES_TO_RADIANS(ps->config->mode.B.rotatePerSecond + ps->config->mode.B.rotatePerSecondVar * RANDOM_M11(&RANDSEED));
	}
}

static void _addParticle(struct particle_system *ps) {
	struct particle *particle;
	if (ps->particleCount == ps->config->totalParticles) {
		return;
	}
	particle = &ps->particles[ps->particleCount];
	_initParticle(ps, particle);
	++ps->particleCount;
}

static void _stopSystem(struct particle_system *ps) {
	ps->isActive = 0;
	ps->elapsed = ps->config->duration;
	ps->emitCounter = 0;
}

static void _normalize_point(struct point *p, struct point *out) {
	float l2 = p->x * p->x + p->y *p->y;
	if (l2 == 0) {
		out->x = 1.0f;
		out->y = 0;
	} else {
		float len = sqrtf(l2);
		out->x = p->x / len;
		out->y = p->y / len;
	}
}

static void _update_particle(struct particle_system *ps, struct particle *p, float dt) {
	if (ps->config->positionType != POSITION_TYPE_GROUPED) {
		p->startPos = ps->config->sourcePosition;
	}
	// Mode A: gravity, direction, tangential accel & radial accel
	if (ps->config->emitterMode == PARTICLE_MODE_GRAVITY) {
		struct point tmp, radial, tangential;
		float newy;

		radial.x = 0;
		radial.y = 0;
		// radial acceleration
		if (p->pos.x || p->pos.y) {
			_normalize_point(&p->pos, &radial);
		}
		tangential = radial;
		radial.x *= p->mode.A.radialAccel;
		radial.y *= p->mode.A.radialAccel;

		// tangential acceleration
		newy = tangential.x;
		tangential.x = -tangential.y * p->mode.A.tangentialAccel;
		tangential.y = newy * p->mode.A.tangentialAccel;

		// (gravity + radial + tangential) * dt
		tmp.x = radial.x + tangential.x + ps->config->mode.A.gravity.x;
		tmp.y = radial.y + tangential.y + ps->config->mode.A.gravity.y;
		tmp.x *= dt;
		tmp.y *= dt;
		p->mode.A.dir.x += tmp.x;
		p->mode.A.dir.y += tmp.y;
		tmp.x = p->mode.A.dir.x * dt;
		tmp.y = p->mode.A.dir.y * dt;
		p->pos.x += tmp.x;
		p->pos.y += tmp.y;
	}
	// Mode B: radius movement
	else {
		// Update the angle and radius of the particle.
		p->mode.B.angle += p->mode.B.degreesPerSecond * dt;
		p->mode.B.radius += p->mode.B.deltaRadius * dt;
		p->pos.x = -cosf(p->mode.B.angle) * p->mode.B.radius;
		p->pos.y = -sinf(p->mode.B.angle) * p->mode.B.radius;
	}
	p->size += (p->deltaSize * dt);
	if (p->size < 0)
		p->size = 0;
	p->color.r += (p->deltaColor.r * dt);
	p->color.g += (p->deltaColor.g * dt);
	p->color.b += (p->deltaColor.b * dt);
	p->color.a += (p->deltaColor.a * dt);
	p->rotation += (p->deltaRotation * dt);
}

static void _remove_particle(struct particle_system *ps, int idx) {
	if (idx != ps->particleCount - 1) {
		ps->particles[idx] = ps->particles[ps->particleCount - 1];
	}
	--ps->particleCount;
}

void init_with_particles(struct particle_system *ps, int numberOfParticles) {
	ps->particles = (struct particle *)(ps + 1);
	ps->matrix = (struct matrix *)(ps->particles + numberOfParticles);
	ps->config = (struct particle_config*)(ps->matrix + numberOfParticles);
	ps->allocatedParticles = numberOfParticles;
	ps->isActive = 0;
	ps->config->totalParticles = numberOfParticles;
	ps->config->positionType = POSITION_TYPE_RELATIVE;
	ps->config->emitterMode = PARTICLE_MODE_GRAVITY;
}

void particle_system_reset(struct particle_system *ps) {
	ps->isActive = 1;
	ps->emitCounter = 0.0;
	ps->particleCount = 0;
	ps->elapsed = 0.0;
}

void calc_particle_system_mat(struct particle * p, struct matrix *m, int edge) {
	struct srt srt;
	struct matrix tmp;

	matrix_identity(m);
	srt.rot = (int)(p->rotation * 1024 / 360);
	srt.scalex = (int)(p->size * 1024 / edge);
	srt.scaley = srt.scalex;
	srt.offx = (int)((p->pos.x + p->startPos.x) * SCREEN_SCALE);
	srt.offy = (int)((p->pos.y + p->startPos.y) * SCREEN_SCALE);
	matrix_srt(m, &srt);
	memcpy(tmp.m, m, sizeof(int) * 6);
	matrix_mul(m, &tmp, &p->emitMatrix);
}

void particle_system_update(struct particle_system *ps, float dt) {
	int i = 0;
	if (ps->isActive) {
		float rate = ps->config->emissionRate;
		// emitCounter should not increase where ps->particleCount == ps->totalParticles
		if (ps->particleCount < ps->config->totalParticles) {
			ps->emitCounter += dt;
		}
		while (ps->particleCount < ps->config->totalParticles && ps->emitCounter > rate) {
			_addParticle(ps);
			ps->emitCounter -= rate;
		}
		ps->elapsed += dt;
		if (ps->config->duration != DURATION_INFINITY && ps->config->duration < ps->elapsed) {
			_stopSystem(ps);
		}
	}
	while (i < ps->particleCount) {
		struct particle *p = &ps->particles[i];
		p->timeToLive -= dt;
		if (p->timeToLive > 0) {
			_update_particle(ps, p, dt);
			if (p->size <= 0) {
				_remove_particle(ps, i);
			} else {
				++i;
			}
		} else {
			_remove_particle(ps, i);
		}
	}
	ps->isAlive = ps->particleCount > 0;
}


#ifdef PIXEL_LUA

#include "lua.h"
#include "lauxlib.h"

static float dict_float(lua_State *L, const char *key) {
	double v;
	lua_getfield(L, -1, key);
	v = lua_tonumber(L, -1);
	lua_pop(L, 1);
	return (float)v;
}

static int dict_int(lua_State *L, const char *key) {
	int v;
	lua_getfield(L, -1, key);
	v = (int)lua_tointeger(L, -1);
	lua_pop(L, 1);
	return v;
}

static uint32_t color4f(struct color4f *c4f) {
	uint8_t rr = (int)(c4f->r * 255);
	uint8_t gg = (int)(c4f->g * 255);
	uint8_t bb = (int)(c4f->b * 255);
	uint8_t aa = (int)(c4f->a * 255);
	return (uint32_t)aa << 24 | (uint32_t)rr << 16 | (uint32_t)gg << 8 | bb;
}

static int _init_from_table(struct particle_config *ps, struct lua_State *L) {
	ps->angle = -dict_float(L, "angle");
	ps->angleVar = -dict_float(L, "angleVariance");
	ps->duration = dict_float(L, "duration");

	// avoid defined blend func
	ps->srcBlend = (int)dict_float(L, "blendFuncSource");
	ps->dstBlend = (int)dict_float(L, "blendFuncDestination");

	ps->startColor.r = dict_float(L, "startColorRed");
	ps->startColor.g = dict_float(L, "startColorGreen");
	ps->startColor.b = dict_float(L, "startColorBlue");
	ps->startColor.a = dict_float(L, "startColorAlpha");
	ps->startColorVar.r = dict_float(L, "startColorVarianceRed");
	ps->startColorVar.g = dict_float(L, "startColorVarianceGreen");
	ps->startColorVar.b = dict_float(L, "startColorVarianceBlue");
	ps->startColorVar.a = dict_float(L, "startColorVarianceAlpha");
	ps->endColor.r = dict_float(L, "finishColorRed");
	ps->endColor.g = dict_float(L, "finishColorGreen");
	ps->endColor.b = dict_float(L, "finishColorBlue");
	ps->endColor.a = dict_float(L, "finishColorAlpha");
	ps->endColorVar.r = dict_float(L, "finishColorVarianceRed");
	ps->endColorVar.g = dict_float(L, "finishColorVarianceGreen");
	ps->endColorVar.b = dict_float(L, "finishColorVarianceBlue");
	ps->endColorVar.a = dict_float(L, "finishColorVarianceAlpha");

	ps->startSize = dict_float(L, "startParticleSize");
	ps->startSizeVar = dict_float(L, "startParticleSizeVariance");
	ps->endSize = dict_float(L, "finishParticleSize");
	ps->endSizeVar = dict_float(L, "finishParticleSizeVariance");

	ps->sourcePosition.x = dict_float(L, "sourcePositionx");
	ps->sourcePosition.y = dict_float(L, "sourcePositiony");

	ps->posVar.x = dict_float(L, "sourcePositionVariancex");
	ps->posVar.y = dict_float(L, "sourcePositionVariancey");

	ps->startSpin = dict_float(L, "rotationStart");
	ps->startSpinVar = dict_float(L, "rotationStartVariance");
	ps->endSpin = dict_float(L, "rotationEnd");
	ps->endSpinVar = dict_float(L, "rotationEndVariance");

	ps->emitterMode = dict_int(L, "emitterType");

	// Mode A: Gravity + tangential accel + radial accel
	if (ps->emitterMode == PARTICLE_MODE_GRAVITY) {
		ps->mode.A.gravity.x = dict_float(L, "gravityx");
		ps->mode.A.gravity.y = -dict_float(L, "gravityy");

		ps->mode.A.speed = dict_float(L, "speed");
		ps->mode.A.speedVar = dict_float(L, "speedVariance");

		ps->mode.A.radialAccel = dict_float(L, "radialAcceleration");
		ps->mode.A.radialAccelVar = dict_float(L, "radialAccelVariance");

		ps->mode.A.tangentialAccel = dict_float(L, "tangentialAcceleration");
		ps->mode.A.tangentialAccelVar = dict_float(L, "tangentialAccelVariance");

		ps->mode.A.rotationIsDir = dict_int(L, "rotationIsDir");
	}
	// or Mode B: radius movement
	else if (ps->emitterMode == PARTICLE_MODE_RADIUS) {
		ps->mode.B.startRadius = dict_float(L, "maxRadius");
		ps->mode.B.startRadiusVar = dict_float(L, "maxRadiusVariance");
		ps->mode.B.endRadius = dict_float(L, "minRadius");
		ps->mode.B.endRadiusVar = dict_float(L, "minRadiusVariance");
		ps->mode.B.rotatePerSecond = dict_float(L, "rotatePerSecond");
		ps->mode.B.rotatePerSecondVar = dict_float(L, "rotatePerSecondVariance");
	} else {
		return 0;
	}
	ps->positionType = dict_int(L, "positionType");
	ps->life = dict_float(L, "particleLifespan");
	ps->lifeVar = dict_float(L, "particleLifespanVariance");
	ps->emissionRate = ps->life / ps->totalParticles;

	return 1;
}

static struct particle_system *_new(struct lua_State *L) {
	int maxParticles = dict_int(L, "maxParticles");
	int totalsize = sizeof(struct particle_system) + maxParticles * (sizeof(struct particle) + sizeof(struct matrix)) + sizeof(struct particle_config);
	struct particle_system * ps = (struct particle_system *)lua_newuserdata(L, totalsize);
	lua_insert(L, -2);
	memset(ps, 0, totalsize);
	init_with_particles(ps, maxParticles);
	if (!_init_from_table(ps->config, L)) {
		lua_pop(L, 2);
		return NULL;
	}
	lua_pop(L, 1);
	return ps;
}

static int lnew(lua_State *L) {
	struct particle_system * ps = _new(L);
	if (ps == NULL)
		return 0;
	return 1;
}

static int lreset(lua_State *L) {
	struct particle_system *ps = (struct particle_system *)lua_touserdata(L, 1);
	particle_system_reset(ps);
	return 1;
}

static int lupdate(lua_State *L) {
	struct particle_system *ps = (struct particle_system *)lua_touserdata(L, 1);
	float dt = (float)luaL_checknumber(L, 2);
	struct matrix *anchor = (struct matrix*)lua_touserdata(L, 3);
	int edge = (int)luaL_checkinteger(L, 4);

	if (ps->config->positionType == POSITION_TYPE_GROUPED) {
		ps->config->emitterMatrix = anchor;
	} else {
		ps->config->emitterMatrix = NULL;
	}
	ps->config->sourcePosition.x = 0;
	ps->config->sourcePosition.y = 0;
	particle_system_update(ps, dt);
	if (ps->isActive || ps->isAlive) {
		int n = ps->particleCount;
		int i;
		struct matrix tmp;
		for (i = 0; i < n; i++) {
			struct particle *p = &ps->particles[i];
			calc_particle_system_mat(p, &ps->matrix[i], edge);
			if (ps->config->positionType != POSITION_TYPE_GROUPED) {
				memcpy(tmp.m, &ps->matrix[i], sizeof(int) * 6);
				matrix_mul(&ps->matrix[i], &tmp, anchor);
			}
			p->color_val = color4f(&p->color);
		}
		lua_pushboolean(L, 1);
	} else {
		lua_pushboolean(L, 0);
	}
	return 1;
}

static int ldata(lua_State *L) {
	struct particle_system *ps = (struct particle_system *)lua_touserdata(L, 1);
	int edge = (int)luaL_checkinteger(L, 4);
	int n = ps->particleCount;
	int i;
	for (i = 0; i < n; i++) {
		uint32_t c;
		struct particle *p = &ps->particles[i];
		calc_particle_system_mat(p, &ps->matrix[i], edge);
		lua_pushlightuserdata(L, &ps->matrix[i]);
		lua_rawseti(L, 2, i + 1);
		c = color4f(&p->color);
		lua_pushinteger(L, c);
		lua_rawseti(L, 3, i + 1);
	}
	lua_pushinteger(L, n);
	return 1;
}

int pixel_particle(lua_State *L) {
	luaL_Reg l[] = {
		{"new", lnew},
		{"reset", lreset},
		{"update", lupdate},
		{"data", ldata},
		{0, 0},
	};
	luaL_newlib(L, l);
	return 1;
}

#endif // PIXEL_LUA