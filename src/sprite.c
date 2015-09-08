#include "sprite.h"
#include "matrix.h"
#include "spritepack.h"
#include "texture.h"
#include "screen.h"
#include "scissor.h"
#include "material.h"
#include "renderbuffer.h"
#include "shader.h"
#include "particle.h"
#include "label.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <limits.h>
#include <malloc.h>

struct material;

struct anchor_data {
	struct particle_system *ps;
	struct pack_picture *pic;
	struct matrix mat;
};

struct sprite {
	struct sprite *parent;
	uint16_t type;
	uint16_t id;
	struct sprite_trans t;
	union {
		struct pack_animation *ani;
		struct pack_picture *pic;
		struct pack_polygon *poly;
		struct pack_label *label;
		struct pack_panel *panel;
		struct matrix *mat;
	} s;
	struct matrix mat;
	int start_frame;
	int total_frame;
	int frame;
	int flag;
	const char *name;
	struct material *material;
	union {
		struct sprite *children[1];
		struct rich_text *rich_text;
		int scissor;
		struct anchor_data *anchor;
	} data;
};

void sprite_free(struct sprite *s) {
	if (!s) {
		return;
	}
	if (s->type == TYPE_ANIMATION) {
		int i;
		struct pack_animation *ani = s->s.ani;
		for (i = 0; i < ani->component_n; i++) {
			struct sprite *c = s->data.children[i];
			if (c) {
				sprite_free(c);
			}
		}
	} else if (s->type == TYPE_LABEL) {
		if (s->data.rich_text) {
			free(s->data.rich_text);
		}
	}
	free(s);
}

static void update_message(struct sprite_pack *pack, struct sprite *s, int parent, int component, int frame) {
	int i;
	struct pack_frame pf;
	struct pack_animation *ani = (struct pack_animation *)pack->data[parent];
	if (frame < 0 || frame >= ani->frame_n) {
		return;
	}
	pf = ani->frame[frame];
	for (i = 0; i < pf.n; i++) {
		if (pf.part[i].component_id == component && pf.part[i].touchable) {
			s->flag |= SPRITE_FLAG_MESSAGE;
			return;
		}
	}
}

static struct sprite *_anchor_new(struct sprite *s, struct sprite_pack *pack, int id) {
	s->parent = 0;
	s->t.mat = 0;
	s->t.color = 0xffffffff;
	s->t.addi = 0;
	s->t.pid = PROGRAM_DEFAULT;
	s->flag = SPRITE_FLAG_INVISIBLE;
	s->name = 0;
	s->id = ANCHOR_ID;
	s->type = TYPE_ANCHOR;
	s->data.anchor = (struct anchor_data *)(s + 1);
	s->data.anchor->ps = 0;
	s->data.anchor->pic = 0;
	s->s.mat = &s->data.anchor->mat;
	s->material = 0;
	matrix_identity(s->s.mat);
	return s;
}

static struct sprite *_new(struct sprite_pack *pack, int id) {
	int i;
	int size;
	struct sprite *s;
	if (id == ANCHOR_ID) {
		size = sizeof(struct sprite) + sizeof(struct anchor_data);
		s = (struct sprite *)malloc(size);
		return _anchor_new(s, pack, id);
	}
	if (pack->type[id] == TYPE_ANIMATION) {
		struct pack_animation *ani = (struct pack_animation *)pack->data[id];
		size = sizeof(struct sprite) + (ani->component_n - 1)*sizeof(struct sprite *);
	} else {
		size = sizeof(struct sprite);
	}
	s = (struct sprite *)malloc(size);
	s->parent = 0;
	s->t.mat = 0;
	s->t.color = 0xffffffff;
	s->t.addi = 0;
	s->t.pid = PROGRAM_DEFAULT;
	s->flag = 0;
	s->name = 0;
	s->id = id;
	s->type = pack->type[id];
	s->material = 0;
	if (pack->type[id] == TYPE_ANIMATION) {
		s->s.ani = (struct pack_animation *)pack->data[id];
		s->frame = 0;
		s->start_frame = 0;
		s->total_frame = 0;
		sprite_action(s, 0);
		for (i = 0; i < s->s.ani->component_n; i++) {
			s->data.children[i] = 0;
		}
	} else {
		s->s.pic = (struct pack_picture *)pack->data[id];
		s->start_frame = 0;
		s->total_frame = 0;
		s->frame = 0;
		memset(&s->data, 0, sizeof(s->data));
		if (s->type == TYPE_PANEL) {
			struct pack_panel *pp = (struct pack_panel *)pack->data[id];
			s->data.scissor = pp->scissor;
		}
	}
	for (i = 0; ; i++) {
		struct sprite *cs;
		int cid = sprite_component(s, i);
		if (cid < 0) {
			break;
		}
		cs = _new(pack, cid);
		if (cs) {
			cs->name = sprite_childname(s, i);
			sprite_mount(s, i, cs);
			update_message(pack, cs, id, i, s->frame);
		}
	}
	return s;
}

struct sprite *sprite_new(const char *packname, const char *name) {
	int id;
	struct sprite_pack *pack = spritepack_query(packname);
	if (!pack) {
		pack = spritepack_load(packname);
	}
	id = spritepack_id(packname, name);
	if (!pack || id == -1) {
		return 0;
	}
	return _new(pack, id);
}

int sprite_action(struct sprite *s, const char *action) {
	struct pack_animation *ani;
	if (s->type != TYPE_ANIMATION) {
		return -1;
	}
	ani = s->s.ani;
	if (action == 0) {
		if (ani->action == 0) {
			return -1;
		}
		s->start_frame = ani->action[0].start_frame;
		s->total_frame = ani->action[0].n;
		s->frame = 0;
		return s->total_frame;
	} else {
		int i;
		for (i = 0; i < ani->action_n; i++) {
			const char *name = (const char *)ani->action[i].name;
			if (name) {
				if (0 == strcmp(name, action)) {
					s->start_frame = ani->action[i].start_frame;
					s->total_frame = ani->action[i].n;
					s->frame = 0;
					return s->total_frame;
				}
			}
		}
		return -1;
	}
}

static inline unsigned int clamp(unsigned int c) {
	return ((c) > 255 ? 255 : (c));
}

static inline uint32_t color_mul(uint32_t c1, uint32_t c2) {
	int r1 = (c1 >> 24) & 0xff;
	int g1 = (c1 >> 16) & 0xff;
	int b1 = (c1 >> 8) & 0xff;
	int a1 = (c1)& 0xff;
	int r2 = (c2 >> 24) & 0xff;
	int g2 = (c2 >> 16) & 0xff;
	int b2 = (c2 >> 8) & 0xff;
	int a2 = c2 & 0xff;

	return (r1 * r2 / 255) << 24 |
		(g1 * g2 / 255) << 16 |
		(b1 * b2 / 255) << 8 |
		(a1 * a2 / 255);
}

static inline uint32_t color_add(uint32_t c1, uint32_t c2) {
	int r1 = (c1 >> 16) & 0xff;
	int g1 = (c1 >> 8) & 0xff;
	int b1 = (c1)& 0xff;
	int r2 = (c2 >> 16) & 0xff;
	int g2 = (c2 >> 8) & 0xff;
	int b2 = (c2)& 0xff;
	return clamp(r1 + r2) << 16 |
		clamp(g1 + g2) << 8 |
		clamp(b1 + b2);
}

static struct sprite_trans *sprite_trans_mul(struct sprite_trans *a, struct sprite_trans *b, struct sprite_trans *t, struct matrix *temp_mat) {
	if (!b) {
		return a;
	}
	*t = *a;
	if (!t->mat) {
		t->mat = b->mat;
	} else if (b->mat) {
		matrix_mul(temp_mat, t->mat, b->mat);
		t->mat = temp_mat;
	}
	if (t->color == 0xffffffff) {
		t->color = b->color;
	} else if (b->color != 0xffffffff) {
		t->color = color_mul(t->color, b->color);
	}
	if (t->addi == 0) {
		t->addi = b->addi;
	} else if (b->addi != 0) {
		t->addi = color_add(t->addi, b->addi);
	}
	if (t->pid == PROGRAM_DEFAULT) {
		t->pid = b->pid;
	}
	return t;
}

static void switch_program(struct sprite_trans *t, int def, struct material *m) {
	int pid = t->pid;
	if (pid == PROGRAM_DEFAULT) {
		pid = def;
	}
	shader_program(pid, m);
}

void sprite_drawquad(struct pack_picture *pic, const struct srt *srt, const struct sprite_trans *arg) {
	struct matrix tmp;
	struct vertex_pack vb[4];
	int i, j;
	int *m;
	if (!pic) {
		return;
	}
	if (!arg->mat) {
		matrix_identity(&tmp);
	} else {
		tmp = *arg->mat;
	}
	matrix_srt(&tmp, srt);
	m = tmp.m;
	for (i = 0; i < pic->n; i++) {
		struct pack_quad *q = &pic->rect[i];
		int glid = texture_rid(q->texid);
		if (glid == 0)
			continue;
		shader_texture(glid, 0);
		for (j = 0; j < 4; j++) {
			int xx = q->screen_coord[j * 2 + 0];
			int yy = q->screen_coord[j * 2 + 1];
			float vx = (xx * m[0] + yy * m[2]) / 1024.0f + m[4];
			float vy = (xx * m[1] + yy * m[3]) / 1024.0f + m[5];
			uint16_t tx = q->texture_coord[j * 2 + 0];
			uint16_t ty = q->texture_coord[j * 2 + 1];

			screen_trans(&vx, &vy);
			vb[j].vx = vx;
			vb[j].vy = vy;
			vb[j].tx = tx;
			vb[j].ty = ty;
		}
		shader_drawvertex(vb, arg->color, arg->addi);
	}
}

void sprite_drawpolygon(struct pack_polygon *poly, const struct srt *srt, const struct sprite_trans *arg) {
	struct matrix tmp;
#if defined(_MSC_VER)
	struct vertex_pack *vb;
#endif
	int i, j;
	int *m;
	if (!arg->mat) {
		matrix_identity(&tmp);
	} else {
		tmp = *arg->mat;
	}
	matrix_srt(&tmp, srt);
	m = tmp.m;
	for (i = 0; i < poly->n; i++) {
		struct pack_poly *p = &poly->poly[i];
		int glid = texture_rid(p->texid);
		if (glid == 0)
			continue;
		shader_texture(glid, 0);
#if defined(_MSC_VER)
		vb = _alloca(p->n*sizeof(struct vertex_pack));
#else
		struct vertex_pack vb[p->n];
#endif
		for (j = 0; j < p->n; j++) {
			int xx = p->screen_coord[j * 2 + 0];
			int yy = p->screen_coord[j * 2 + 1];
			float vx = (xx * m[0] + yy * m[2]) / 1024.0f + m[4];
			float vy = (xx * m[1] + yy * m[3]) / 1024.0f + m[5];
			uint16_t tx = p->texture_coord[j * 2 + 0];
			uint16_t ty = p->texture_coord[j * 2 + 1];

			screen_trans(&vx, &vy);
			vb[j].vx = vx;
			vb[j].vy = vy;
			vb[j].tx = tx;
			vb[j].ty = ty;
		}
		shader_drawpolygon(p->n, vb, arg->color, arg->addi);
	}
}

static int get_frame(struct sprite *s) {
	int f;
	if (s->type != TYPE_ANIMATION) {
		return s->start_frame;
	}
	if (s->total_frame <= 0) {
		return -1;
	}
	f = s->frame % s->total_frame;
	if (f < 0) {
		f += s->total_frame;
	}
	return f + s->start_frame;
}

static void set_scissor(const struct pack_panel *pp, const struct srt *srt, const struct sprite_trans *tran) {
	int *m;
	int x[4], y[4], i;
	int minx, miny, maxx, maxy;
	struct matrix tmp;
	if (!tran->mat) {
		matrix_identity(&tmp);
	} else {
		tmp = *tran->mat;
	}
	matrix_srt(&tmp, srt);
	m = tmp.m;
	x[0] = 0;
	x[1] = pp->width*SCREEN_SCALE;
	x[2] = pp->width*SCREEN_SCALE;
	x[3] = 0;
	y[0] = 0;
	y[1] = 0;
	y[2] = pp->height*SCREEN_SCALE;
	y[3] = pp->height*SCREEN_SCALE;
	minx = (x[0] * m[0] + y[0] * m[2]) / 1024 + m[4];
	miny = (x[0] * m[1] + y[0] * m[3]) / 1024 + m[5];
	maxx = minx;
	maxy = miny;
	for (i = 1; i < 4; i++) {
		int vx = (x[i] * m[0] + y[i] * m[2]) / 1024 + m[4];
		int vy = (x[i] * m[1] + y[i] * m[3]) / 1024 + m[5];
		if (vx < minx) {
			minx = vx;
		} else if (vx > maxx) {
			maxx = vx;
		}
		if (vy < miny) {
			miny = vy;
		} else if (vy > maxy) {
			maxy = vy;
		}
	}
	minx /= SCREEN_SCALE;
	miny /= SCREEN_SCALE;
	maxx /= SCREEN_SCALE;
	maxy /= SCREEN_SCALE;
	scissor_push(minx, miny, maxx - minx, maxy - miny);
}

static void anchor_update(struct sprite *s, struct srt *srt, struct sprite_trans *arg) {
	struct matrix *t = s->s.mat;
	if (!arg->mat) {
		matrix_identity(t);
	} else {
		*t = *arg->mat;
	}
	matrix_srt(t, srt);
}

static void sprite_drawparticle(struct sprite *s, struct particle_system *ps, struct pack_picture *pic, const struct srt *srt) {
	int i;
	int n = ps->particleCount;
	struct matrix *old_m = s->t.mat;
	uint32_t old_c = s->t.color;

	shader_blend(ps->config->srcBlend, ps->config->dstBlend);
	for (i = 0; i < n; i++) {
		struct particle *p = &ps->particles[i];
		struct matrix *mat = &ps->matrix[i];
		uint32_t color = p->color_val;

		s->t.mat = mat;
		s->t.color = color;
		sprite_drawquad(pic, 0, &s->t);
	}
	shader_default_blend();
	s->t.mat = old_m;
	s->t.color = old_c;
}

static void _draw_ani(struct sprite *s, struct srt *srt, struct material *material, struct sprite_trans *t);

static int draw_child(struct sprite *s, struct srt *srt, struct sprite_trans *ts, struct material *material) {
	struct sprite_trans temp;
	struct matrix temp_mat;
	struct sprite_trans *t = sprite_trans_mul(&s->t, ts, &temp, &temp_mat);
	if (s->material) {
		material = s->material;
	}
	switch (s->type) {
	case TYPE_PICTURE:
		switch_program(t, PROGRAM_PICTURE, material);
		sprite_drawquad(s->s.pic, srt, t);
		return 0;
	case TYPE_POLYGON:
		switch_program(t, PROGRAM_PICTURE, material);
		sprite_drawpolygon(s->s.poly, srt, t);
		return 0;
	case TYPE_LABEL:
		if (s->data.rich_text) {
			t->pid = PROGRAM_DEFAULT;
			switch_program(t, s->s.label->edge ? PROGRAM_TEXT_EDGE : PROGRAM_TEXT, material);
			label_draw(s->data.rich_text, s->s.label, srt, t);
		}
		return 0;
	case TYPE_ANCHOR:
		if (s->data.anchor->ps) {
			switch_program(t, PROGRAM_PICTURE, material);
			sprite_drawparticle(s, s->data.anchor->ps, s->data.anchor->pic, srt);
		}
		anchor_update(s, srt, t);
		return 0;
	case TYPE_PANEL:
		if (s->data.scissor) {
			set_scissor(s->s.panel, srt, t);
			return 1;
		} else {
			return 0;
		}
	case TYPE_ANIMATION:
		break;
	default:
		return 0;
	}
	_draw_ani(s, srt, material, t);
	return 0;
}

static void _draw_ani(struct sprite *s, struct srt *srt, struct material *material, struct sprite_trans *t) {
	int i, scissor = 0;
	struct pack_frame *pf;
	struct pack_animation *ani = s->s.ani;
	int frame = get_frame(s);
	if (frame < 0) {
		return;
	}
	pf = &ani->frame[frame];
	for (i = 0; i < pf->n; i++) {
		struct sprite_trans tran;
		struct matrix mat;
		struct sprite_trans *ct;
		struct pack_part *pp = &pf->part[i];
		int idx = pp->component_id;
		struct sprite *child = s->data.children[idx];
		if (!child || (child->flag & SPRITE_FLAG_INVISIBLE)) {
			continue;
		}
		ct = sprite_trans_mul(&pp->t, t, &tran, &mat);
		scissor += draw_child(child, srt, ct, material);
	}
	for (i = 0; i < scissor; i++) {
		scissor_pop();
	}
}

void sprite_draw(struct sprite *s, struct srt *srt) {
	if (!s) {
		return;
	}
	if ((s->flag & SPRITE_FLAG_INVISIBLE) == 0) {
		draw_child(s, srt, 0, 0);
	}
}

void sprite_ps(struct sprite *s, int x, int y, float scale) {
	int *mat;
	struct matrix *m = &s->mat;
	if (!s) {
		return;
	}
	if (!s->t.mat) {
		matrix_identity(m);
		s->t.mat = m;
	}
	mat = m->m;
	x *= SCREEN_SCALE;
	y *= SCREEN_SCALE;
	if (scale > 0) {
		scale *= 1024;
		mat[0] = (int)scale;
		mat[1] = 0;
		mat[2] = 0;
		mat[3] = (int)scale;
	}
	mat[4] = x;
	mat[5] = y;
}

void sprite_scale(struct sprite *s, float scale) {
	int *mat;
	struct matrix *m;
	if (!s) {
		return;
	}
	m = &s->mat;
	if (!s->t.mat) {
		matrix_identity(m);
		s->t.mat = m;
	}
	mat = m->m;
	scale *= 1024;
	mat[0] = (int)scale;
	mat[1] = 0;
	mat[2] = 0;
	mat[3] = (int)scale;
}

void sprite_sr(struct sprite *s, float sx, float sy, float r) {
	struct matrix *mat;
	if (!s) {
		return;
	}
	mat = &s->mat;
	if (!s->t.mat) {
		matrix_identity(mat);
		s->t.mat = mat;
	}
	sx *= 1024;
	sy *= 1024;
	r *= (1024.0f / 360.f);
	matrix_sr(mat, (int)sx, (int)sy, (int)r);
}

void sprite_rot(struct sprite *s, float r) {
	struct matrix *mat;
	if (!s) {
		return;
	}
	mat = &s->mat;
	if (!s->t.mat) {
		matrix_identity(mat);
		s->t.mat = mat;
	}
	r *= (1024.0f / 360.f);
	matrix_sr(mat, 1024, 1024, (int)r);
}

void sprite_pmatrix(struct sprite *s, struct matrix *mat) {
	struct sprite *p = s->parent;
	if (p) {
		struct matrix tmp;
		struct matrix *pmat, *cmat;
		struct pack_animation *ani = p->s.ani;
		struct pack_frame *pf;
		int frame, i;
		assert(p->type == TYPE_ANIMATION);
		sprite_pmatrix(p, mat);
		pmat = p->t.mat;
		cmat = 0;
		frame = get_frame(p);
		if (frame < 0) {
			return;
		}
		pf = &ani->frame[frame];
		for (i = 0; i < pf->n; i++) {
			struct pack_part *pp = &pf->part[i];
			int idx = pp->component_id;
			struct sprite *child = p->data.children[idx];
			if (child == s) {
				cmat = pp->t.mat;
				break;
			}
		}
		if (!pmat && !cmat) {
			return;
		}
		if (pmat) {
			matrix_mul(&tmp, pmat, mat);
		} else {
			tmp = *mat;
		}
		if (cmat) {
			matrix_mul(mat, cmat, &tmp);
		} else {
			*mat = tmp;
		}
	} else {
		matrix_identity(mat);
	}
}

struct matrix *sprite_worldmatrix(struct sprite *s) {
	if (s->type != TYPE_ANCHOR) {
		return 0;
	}
	return s->s.mat;
}

struct matrix *sprite_matrix(struct sprite *s, struct matrix *mat) {
	struct matrix *oldmat;

	if (!s->t.mat) {
		s->t.mat = &s->mat;
		matrix_identity(&s->mat);
	}
	oldmat = s->t.mat;
	if (!mat) {
		return oldmat;
	}
	s->t.mat = &s->mat;
	s->mat = *mat;
	return oldmat;
}

static struct matrix *mat_mul(struct matrix *a, struct matrix *b, struct matrix *tmp) {
	if (!b) {
		return a;
	}
	if (!a) {
		return b;
	}
	matrix_mul(tmp, a, b);
	return tmp;
}

static void poly_aabb(int n, const int32_t *point, struct srt *srt, struct matrix *mat, int aabb[4]) {
	int *m;
	int i;
	struct matrix tmp;
	if (!mat) {
		matrix_identity(&tmp);
	} else {
		tmp = *mat;
	}
	matrix_srt(&tmp, srt);
	m = tmp.m;
	for (i = 0; i < n; i++) {
		int x = point[i * 2];
		int y = point[i * 2 + 1];
		int xx = (x*m[0] + y*m[2]) / 1024 + m[4];
		int yy = (x*m[1] + y*m[3]) / 1024 + m[5];

		if (xx < aabb[0]) {
			aabb[0] = xx;
		}
		if (xx > aabb[2]) {
			aabb[2] = xx;
		}
		if (yy < aabb[1]) {
			aabb[1] = yy;
		}
		if (yy > aabb[3]) {
			aabb[3] = yy;
		}
	}
}

static inline void quad_aabb(struct pack_picture *pic, struct srt *srt, struct matrix *mat, int aabb[4]) {
	int i;
	for (i = 0; i < pic->n; i++) {
		poly_aabb(4, pic->rect[i].screen_coord, srt, mat, aabb);
	}
}

static inline void polygon_aabb(struct pack_polygon *polygon, struct srt *srt, struct matrix *mat, int aabb[4]) {
	int i;
	for (i = 0; i < polygon->n; i++) {
		struct pack_poly *poly = &polygon->poly[i];
		poly_aabb(poly->n, poly->screen_coord, srt, mat, aabb);
	}
}

static inline void label_aabb(struct pack_label *lab, struct srt *srt, struct matrix *mat, int aabb[4]) {
	int32_t pt[] = {
		0,0,lab->width*SCREEN_SCALE,0,
		0,lab->height*SCREEN_SCALE, lab->width*SCREEN_SCALE, lab->height*SCREEN_SCALE,
	};
	poly_aabb(4, pt, srt, mat, aabb);
}

static inline void panel_aabb(struct pack_panel *pp, struct srt *srt, struct matrix *mat, int aabb[4]) {
	int32_t pt[] = {
		0,0,pp->width*SCREEN_SCALE,0,
		0,pp->height*SCREEN_SCALE,pp->width*SCREEN_SCALE,pp->height*SCREEN_SCALE,
	};
	poly_aabb(4, pt, srt, mat, aabb);
}

static int child_aabb(struct sprite *s, struct srt *srt, struct matrix *mat, int aabb[4]) {
	struct pack_animation *ani;
	int frame, i;
	struct pack_frame *pf;
	struct matrix tmp;
	struct matrix *t = mat_mul(s->t.mat, mat, &tmp);
	switch (s->type) {
	case TYPE_PICTURE:
		quad_aabb(s->s.pic, srt, t, aabb);
		return 0;
	case TYPE_POLYGON:
		polygon_aabb(s->s.poly, srt, t, aabb);
		return 0;
	case TYPE_LABEL:
		label_aabb(s->s.label, srt, t, aabb);
		return 0;
	case TYPE_PANEL:
		panel_aabb(s->s.panel, srt, t, aabb);
		return s->data.scissor;
	case TYPE_ANIMATION:
		break;
	default:
		return 0;
	}
	ani = s->s.ani;
	frame = get_frame(s);
	if (frame < 0) {
		return 0;
	}
	pf = &ani->frame[frame];
	for (i = 0; i < pf->n; i++) {
		struct matrix *ct;
		struct matrix tmp2;
		struct pack_part *pp = &pf->part[i];
		int idx = pp->component_id;
		struct sprite *child = s->data.children[idx];
		if (!child || (child->flag & SPRITE_FLAG_INVISIBLE)) {
			continue;
		}
		ct = mat_mul(pp->t.mat, t, &tmp2);
		if (child_aabb(child, srt, ct, aabb)) {
			break;
		}
	}
	return 0;
}

void sprite_aabb(struct sprite *s, struct srt *srt, int world, int aabb[4]) {
	if ((s->flag & SPRITE_FLAG_INVISIBLE) == 0) {
		int i;
		struct matrix tmp;
		if (world) {
			sprite_pmatrix(s, &tmp);
		} else {
			matrix_identity(&tmp);
		}
		aabb[0] = INT_MAX;
		aabb[1] = INT_MAX;
		aabb[2] = INT_MIN;
		aabb[3] = INT_MIN;
		child_aabb(s, srt, &tmp, aabb);
		for (i = 0; i < 4; i++) {
			aabb[i] /= SCREEN_SCALE;
		}
	} else {
		int i;
		for (i = 0; i < 4; i++) {
			aabb[i] = 0;
		}
	}
}

static int test_quad(struct pack_picture *pic, int x, int y) {
	int p;
	for (p = 0; p < pic->n; p++) {
		int maxx, maxy, minx, miny, i;
		struct pack_quad *pq = &pic->rect[p];
		minx = maxx = pq->screen_coord[0];
		miny = maxy = pq->screen_coord[1];
		for (i = 2; i < 8; i += 2) {
			int sx = pq->screen_coord[i];
			int sy = pq->screen_coord[i + 1];
			if (sx < minx) {
				minx = sx;
			} else if (sx > maxx) {
				maxx = sx;
			}
			if (sy < miny) {
				miny = sy;
			} else if (sy > maxy) {
				maxy = sy;
			}
		}
		if (x >= minx && x <= maxx && y >= miny && y <= maxy) {
			return 1;
		}
	}
	return 0;
}

static int test_polygon(struct pack_polygon *poly, int x, int y) {
	int p;
	for (p = 0; p < poly->n; p++) {
		struct pack_poly *pp = &poly->poly[p];
		int i, maxx, maxy, minx, miny;
		minx = maxx = pp->screen_coord[0];
		miny = maxy = pp->screen_coord[1];
		for (i = 1; i < pp->n; i++) {
			int xx = pp->screen_coord[i * 2 + 0];
			int yy = pp->screen_coord[i * 2 + 1];
			if (x<minx)
				minx = xx;
			else if (x>maxx)
				maxx = xx;
			if (y<miny)
				miny = yy;
			else if (y>maxy)
				maxy = yy;
		}
		if (x >= minx && x <= maxx && y >= miny && y <= maxy) {
			return 1;
		}
	}
	return 0;
}

static int test_label(struct pack_label *pl, int x, int y) {
	x /= SCREEN_SCALE;
	y /= SCREEN_SCALE;
	return x >= 0 && x < pl->width && y >= 0 && y < pl->height;
}

static int test_panel(struct pack_panel *pp, int x, int y) {
	x /= SCREEN_SCALE;
	y /= SCREEN_SCALE;
	return x >= 0 && x < pp->width && y >= 0 && y < pp->height;
}

static int test_child(struct sprite *s, struct srt *srt, struct matrix *mat, int x, int y, struct sprite **touch);
static int check_child(struct sprite *s, struct srt *srt, struct matrix *t, struct pack_frame *pf, int i, int x, int y, struct sprite **touch) {
	int test;
	struct matrix temp2;
	struct matrix *ct;
	struct sprite *tmp = 0;
	struct pack_part *pp = &pf->part[i];
	int index = pp->component_id;
	struct sprite * child = s->data.children[index];
	if (!child || (child->flag & SPRITE_FLAG_INVISIBLE)) {
		return 0;
	}
	ct = mat_mul(pp->t.mat, t, &temp2);
	test = test_child(child, srt, ct, x, y, &tmp);
	if (test) {
		*touch = tmp;
		return 1;
	}
	if (tmp) {
		*touch = tmp;
	}
	return 0;
}

static int test_animation(struct sprite *s, struct srt *srt, struct matrix *t, int x, int y, struct sprite ** touch) {
	int start;
	struct pack_frame *pf;
	struct pack_animation *ani = s->s.ani;
	int frame = get_frame(s);
	if (frame < 0) {
		return 0;
	}
	pf = &ani->frame[frame];
	start = pf->n - 1;
	do {
		int scissor = -1;
		int i;
		for (i = start; i >= 0; i--) {
			struct pack_part *pp = &pf->part[i];
			int index = pp->component_id;
			struct sprite *c = s->data.children[index];
			if (!c || (c->flag & SPRITE_FLAG_INVISIBLE)) {
				continue;
			}
			if (c->type == TYPE_PANEL && c->data.scissor) {
				scissor = i;
				break;
			}
		}
		if (scissor >= 0) {
			struct sprite *tmp = 0;
			check_child(s, srt, t, pf, scissor, x, y, &tmp);
			if (!tmp) {
				start = scissor - 1;
				continue;
			}
		} else {
			scissor = 0;
		}
		for (i = start; i >= scissor; i--) {
			int hit = check_child(s, srt, t, pf, i, x, y, touch);
			if (hit)
				return 1;
		}
		start = scissor - 1;
	} while (start >= 0);
	return 0;
}

static int test_child(struct sprite *s, struct srt *srt, struct matrix *mat, int x, int y, struct sprite **touch) {
	int *m;
	int xx, yy, test;
	struct sprite *ts;
	struct matrix tmp, imat;
	struct matrix *t = mat_mul(s->t.mat, mat, &tmp);
	if (s->type == TYPE_ANIMATION) {
		struct sprite *spr = 0;
		test = test_animation(s, srt, t, x, y, &spr);
		if (test) {
			*touch = spr;
			return 1;
		} else if (spr) {
			if (s->flag & SPRITE_FLAG_MESSAGE) {
				*touch = s;
				return 1;
			} else {
				*touch = spr;
				return 0;
			}
		}
	}
	if (!t) {
		matrix_identity(&tmp);
	} else {
		tmp = *t;
	}
	matrix_srt(&tmp, srt);
	if (matrix_inverse(&tmp, &imat)) {
		*touch = 0;
		return 0;
	}
	m = imat.m;
	xx = (x*m[0] + y*m[2]) / 1024 + m[4];
	yy = (x*m[1] + y*m[3]) / 1024 + m[5];
	ts = s;
	switch (s->type) {
	case TYPE_PICTURE:
		test = test_quad(s->s.pic, xx, yy);
		break;
	case TYPE_POLYGON:
		test = test_polygon(s->s.poly, xx, yy);
		break;
	case TYPE_LABEL:
		test = test_label(s->s.label, xx, yy);
		break;
	case TYPE_PANEL:
		test = test_panel(s->s.panel, xx, yy);
		break;
	case TYPE_ANCHOR:
		*touch = 0;
		return 0;
	default:
		*touch = 0;
		return 0;
	}
	if (test) {
		*touch = ts;
		return (s->flag & SPRITE_FLAG_MESSAGE);
	} else {
		*touch = 0;
		return 0;
	}
}

struct sprite *sprite_test(struct sprite *s, struct srt *srt, int x, int y) {
	struct sprite *tmp = 0;
	int test;

	x *= SCREEN_SCALE;
	y *= SCREEN_SCALE;
	test = test_child(s, srt, 0, x, y, &tmp);
	if (test) {
		return tmp;
	}
	if (tmp) {
		return s;
	}
	return 0;
}

int sprite_visible(struct sprite *s, int visible) {
	int v = (s->flag & SPRITE_FLAG_INVISIBLE) == 0;
	if (-1 == visible) {
		return v;
	}
	if (visible == 1)
		s->flag &= ~SPRITE_FLAG_INVISIBLE;
	else
		s->flag |= SPRITE_FLAG_INVISIBLE;
	return v;
}

const char *sprite_name(struct sprite *s) {
	return s->name;
}

int sprite_component(struct sprite *s, int idx) {
	struct pack_animation *ani;
	if (s->type != TYPE_ANIMATION) {
		return -1;
	}
	ani = s->s.ani;
	if (idx < 0 || idx >= ani->component_n) {
		return -1;
	}
	return ani->component[idx].id;
}

const char *sprite_childname(struct sprite *s, int idx) {
	struct pack_animation *ani;
	if (s->type != TYPE_ANIMATION) {
		return 0;
	}
	ani = s->s.ani;
	if (idx < 0 || idx >= ani->component_n) {
		return 0;
	}
	return (const char *)ani->component[idx].name;
}

int sprite_children_name(struct sprite *s, const char *names[]) {
	int i, cnt = 0;
	struct pack_animation *ani;
	if (s->type != TYPE_ANIMATION) {
		return 0;
	}
	ani = s->s.ani;
	for (i = 0; i < ani->component_n; i++) {
		if (ani->component[i].name) {
			names[cnt++] = (const char *)ani->component[i].name;
		}
	}
	return cnt;
}

int sprite_child_idx1(struct sprite *s, struct sprite *c) {
	int i;
	struct pack_animation *ani;

	if (s->type != TYPE_ANIMATION) {
		return 0;
	}
	ani = s->s.ani;
	for (i = 0; i < ani->component_n; i++) {
		if (c == s->data.children[i]) {
			return i;
		}
	}
	return -1;
}

int sprite_child_idx(struct sprite *s, const char *name) {
	int i;
	struct pack_animation *ani;

	assert(name);
	if (s->type != TYPE_ANIMATION) {
		return -1;
	}
	ani = s->s.ani;
	for (i = 0; i < ani->component_n; i++) {
		const char *cname = (const char *)ani->component[i].name;
		if (cname && 0 == strcmp(name, cname)) {
			return i;
		}
	}
	return -1;
}

struct sprite *sprite_child(struct sprite *s, const char *name) {
	int i;
	struct pack_animation *ani;

	assert(name);
	if (s->type != TYPE_ANIMATION) {
		return 0;
	}
	ani = s->s.ani;
	for (i = 0; i < ani->component_n; i++) {
		const char *cname = (const char *)ani->component[i].name;
		if (cname && 0 == strcmp(name, cname)) {
			return s->data.children[i];
		}
	}
	return 0;
}

int sprite_child_visible(struct sprite *s, const char *name) {
	int i, frame;
	struct pack_animation *ani;
	struct pack_frame *pf;
	assert(name);
	if (s->type != TYPE_ANIMATION) {
		return 0;
	}
	ani = s->s.ani;
	frame = get_frame(s);
	if (frame < 0) {
		return 0;
	}
	pf = &ani->frame[frame];
	for (i = 0; i < pf->n; i++) {
		struct pack_part *pp = &pf->part[i];
		int idx = pp->component_id;
		struct sprite *child = s->data.children[idx];
		if (child->name && strcmp(name, child->name) == 0) {
			return 1;
		}
	}
	return 0;
}

void sprite_mount(struct sprite *s, int idx, struct sprite *c) {
	struct pack_animation *ani;
	struct sprite *os;

	if (s->type != TYPE_ANIMATION) {
		return;
	}
	ani = s->s.ani;
	os = s->data.children[idx];
	if (os) {
		os->parent = 0;
		os->name = 0;
	}
	s->data.children[idx] = c;
	if (c) {
		assert(c->parent == 0);
		if ((c->flag & SPRITE_FLAG_MULTIMOUNT) == 0) {
			c->name = (const char *)ani->component[idx].name;
			c->parent = s;
		}
		if (os && os->type == TYPE_ANCHOR) {
			if (os->flag & SPRITE_FLAG_MESSAGE)
				c->flag |= SPRITE_FLAG_MESSAGE;
			else
				c->flag &= ~SPRITE_FLAG_MESSAGE;
		}
	}
}

int sprite_frame1(struct sprite *s, int dframe) {
	if (!s || s->type != TYPE_ANIMATION) {
		return 0;
	}
	return sprite_frame(s, s->frame + dframe, 0);
}

int sprite_total_frame(struct sprite *s) {
	return s->total_frame;
}

static inline int force_frame(struct sprite *s, int i, int force) {
	struct sprite *c = s->data.children[i];
	if (!c || c->type != TYPE_ANIMATION) return 0;
	if (c->flag & SPRITE_FLAG_FORCEFRAME) return 1;
	struct pack_animation *ani = s->s.ani;
	if (ani->component[i].id == ANCHOR_ID) return 0;
	if (force) return 1;
	if (!ani->component[i].name) return 1;
	return 0;
}

int sprite_frame(struct sprite *s, int frame, int force) {
	int i, total;
	struct pack_animation *ani;
	if (!s || s->type != TYPE_ANIMATION) {
		return 0;
	}
	if (-1 == frame) {
		return s->frame;
	}
	s->frame = frame;
	total = s->total_frame;
	ani = s->s.ani;
	for (i = 0; i < ani->component_n; i++) {
		if (force_frame(s, i, force)) {
			int t = sprite_frame(s->data.children[i], frame, force);
			if (t > total) {
				total = t;
			}
		}
	}
	return total;
}

const char *sprite_text(struct sprite *s, const char *text) {
	struct rich_text *rich;
	if (s->type != TYPE_LABEL) {
		return 0;
	}
	if (0 == text) {
		if (s->data.rich_text) {
			return s->data.rich_text->text;
		}
		return 0;
	}
	if (0 == strcmp("", text)) {
		if (s->data.rich_text) {
			free(s->data.rich_text);
			s->data.rich_text = 0;
		}
		return 0;
	}
	if (!s->data.rich_text) {
		s->data.rich_text = (struct rich_text *)malloc(sizeof(struct rich_text));
	}
	rich = s->data.rich_text;
	rich->text = text;
	rich->count = 0;
	rich->width = 0;
	rich->height = 0;

	if (rich->count > 0) {
		int i;
		struct label_field *field;
		rich->fields = (struct label_field *)malloc(rich->count * sizeof(struct label_field));
		field = rich->fields;
		for (i = 0; i < rich->count; i++) {
			int type = 0;

			((struct label_field*)(field + i))->start = 0;
			((struct label_field*)(field + i))->end = 0;
			((struct label_field*)(field + i))->type = type;
			if (type == RL_COLOR) {
				((struct label_field*)(field + i))->color = 0;
			} else {
				((struct label_field*)(field + i))->val = 0;
			}
		}
	}
	return 0;
}

int sprite_scissor(struct sprite *s, int scissor) {
	int old_scissor;
	if (s->type != TYPE_PANEL) {
		return -1;
	}
	old_scissor = s->data.scissor;
	s->data.scissor = scissor;
	return old_scissor;
}

static struct sprite *sprite_label_init(struct sprite *s, struct pack_label *pl) {
	struct pack_label *label = (struct pack_label *)(s + 1);
	s->parent = 0;
	*label = *pl;
	s->s.label = label;
	s->t.mat = 0;
	s->t.color = 0xffffffff;
	s->t.addi = 0;
	s->t.pid = PROGRAM_DEFAULT;
	s->flag = 0;
	s->name = 0;
	s->id = 0;
	s->type = TYPE_LABEL;
	s->start_frame = 0;
	s->total_frame = 0;
	s->frame = 0;
	s->data.rich_text = 0;
	s->material = 0;
	return s;
}

struct sprite *sprite_label(struct pack_label *pl, const char *text) {
	struct sprite *s = (struct sprite *)malloc(sizeof(struct sprite) + sizeof(struct pack_label));
	s = sprite_label_init(s, pl);
	if (text) {
		sprite_text(s, text);
	}
	return s;
}

void sprite_draw_triangle(int tid, float p[6], uint32_t color, uint32_t addi) {
	struct sprite_trans trans;
	struct pack_polygon pp;
	int32_t _screen_coord[6];
	uint16_t _texture_coord[6];
	int i;

	pp.n = 1;
	pp.poly[0].n = 3;
	pp.poly[0].texid = tid;
	for (i = 0; i < 3; i++) {
		texture_coord(tid, p[i * 2], p[i * 2 + 1], &_texture_coord[i * 2], &_texture_coord[i * 2 + 1]);
		_screen_coord[i * 2] = (int32_t)(p[i * 2] * 16);
		_screen_coord[i * 2 + 1] = (int32_t)(p[i * 2 + 1] * 16);
	}
	pp.poly[0].screen_coord = _screen_coord;
	pp.poly[0].texture_coord = _texture_coord;
	shader_program(PROGRAM_PICTURE, 0);
	memset(&trans, 0, sizeof trans);
	trans.pid = -1;
	trans.color = color;
	trans.addi = addi;
	sprite_drawpolygon(&pp, 0, &trans);
}

void sprite_particle(struct sprite *s, struct particle_system *ps, struct sprite *a) {
	if (s->type != TYPE_ANCHOR) {
		return;
	}
	s->data.anchor->ps = ps;
	if (a->type != TYPE_PICTURE) {
		return;
	}
	s->data.anchor->pic = a->s.pic;
}


//renderbuffer
static int rb_update_tex(struct renderbuffer *rb, int id) {
	if (rb->object == 0) {
		rb->tid = id;
	} else if (rb->tid != id) {
		return 1;
	}
	return 0;
}

static int rb_drawquad(struct renderbuffer *rb, struct pack_picture *pic, const struct sprite_trans *arg) {
	int *m;
	int object;
	struct matrix tmp;
	struct vertex_pack vb[4];
	int i, j;
	if (arg->mat == NULL) {
		matrix_identity(&tmp);
	} else {
		tmp = *arg->mat;
	}
	m = tmp.m;
	object = rb->object;
	for (i = 0; i < pic->n; i++) {
		struct pack_quad *q = &pic->rect[i];
		if (rb_update_tex(rb, q->texid)) {
			rb->object = object;
			return -1;
		}
		for (j = 0; j < 4; j++) {
			int xx = q->screen_coord[j * 2 + 0];
			int yy = q->screen_coord[j * 2 + 1];
			vb[j].vx = (xx * m[0] + yy * m[2]) / 1024.0f + m[4];
			vb[j].vy = (xx * m[1] + yy * m[3]) / 1024.0f + m[5];
			vb[j].tx = q->texture_coord[j * 2 + 0];
			vb[j].ty = q->texture_coord[j * 2 + 1];
		}
		if (renderbuffer_addvertex(rb, vb, arg->color, arg->addi)) {
			return 1;
		}
	}
	return 0;
}

static int rb_polygon_quad(struct renderbuffer *rb, const struct vertex_pack *vbp, uint32_t color, uint32_t addi, int max, int index) {
	struct vertex_pack vb[4];
	int i;
	vb[0] = vbp[0];
	for (i = 1; i < 4; i++) {
		int j = i + index;
		int n = (j <= max) ? j : max;
		vb[i] = vbp[n];
	}
	return renderbuffer_addvertex(rb, vb, color, addi);
}

static int rb_add_polygon(struct renderbuffer *rb, int n, const struct vertex_pack *vb, uint32_t color, uint32_t addi) {
	int i = 0;
	--n;
	do {
		if (rb_polygon_quad(rb, vb, color, addi, n, i)) {
			return 1;
		}
		i += 2;
	} while (i < n - 1);
	return 0;
}

static int rb_drawpolygon(struct renderbuffer *rb, struct pack_polygon *poly, const struct sprite_trans *arg) {
	struct matrix tmp;
	int i, j;
	int object;
	int *m;
	if (!arg->mat) {
		matrix_identity(&tmp);
	} else {
		tmp = *arg->mat;
	}
	m = tmp.m;
	object = rb->object;
	for (i = 0; i < poly->n; i++) {
#if defined(_MSC_VER)
		struct vertex_pack *vb;
#endif
		struct pack_poly *p = &poly->poly[i];
		if (rb_update_tex(rb, p->texid)) {
			rb->object = object;
			return -1;
		}
#if defined(_MSC_VER)
		vb = (struct vertex_pack *)_alloca(p->n*sizeof(struct vertex_pack));
#else
		struct vertex_pack vb[p->n];
#endif
		for (j = 0; j < p->n; j++) {
			int xx = p->screen_coord[j * 2 + 0];
			int yy = p->screen_coord[j * 2 + 1];
			vb[j].vx = (xx * m[0] + yy * m[2]) / 1024.0f + m[4];
			vb[j].vy = (xx * m[1] + yy * m[3]) / 1024.0f + m[5];
			vb[j].tx = p->texture_coord[j * 2 + 0];
			vb[j].ty = p->texture_coord[j * 2 + 1];
		}
		if (rb_add_polygon(rb, p->n, vb, arg->color, arg->addi)) {
			rb->object = object;
			return 1;
		}
	}
	return 0;
}

static int rb_drawsprite(struct renderbuffer *rb, struct sprite *s, struct sprite_trans *ts) {
	struct pack_animation *ani;
	struct pack_frame * pf;
	int frame, i;
	struct sprite_trans temp;
	struct matrix temp_matrix;
	struct sprite_trans *t = sprite_trans_mul(&s->t, ts, &temp, &temp_matrix);
	switch (s->type) {
	case TYPE_PICTURE:
		return rb_drawquad(rb, s->s.pic, t);
	case TYPE_POLYGON:
		return rb_drawpolygon(rb, s->s.poly, t);
	case TYPE_ANIMATION:
		break;
	case TYPE_LABEL:
		if (s->data.rich_text) {
			// don't support draw label to renderbuffer
			return -1;
		}
		return 0;
	case TYPE_ANCHOR:
		if (s->data.anchor->ps) {
			return -1;
		}
		anchor_update(s, 0, t);
		return 0;
	case TYPE_PANEL:
		if (s->data.scissor) {
			return -1;
		} else {
			return 0;
		}
	default:
		return -1;
	}
	ani = s->s.ani;
	frame = get_frame(s);
	if (frame < 0) {
		return -1;
	}
	pf = &ani->frame[frame];
	for (i = 0; i < pf->n; i++) {
		struct sprite_trans temp2;
		struct matrix temp_matrix2;
		struct sprite_trans *ct;
		struct pack_part *pp = &pf->part[i];
		int index = pp->component_id;
		struct sprite *child = s->data.children[index];
		if (!child || (child->flag & SPRITE_FLAG_INVISIBLE)) {
			continue;
		}
		ct = sprite_trans_mul(&pp->t, t, &temp2, &temp_matrix2);
		if (rb_drawsprite(rb, child, ct)) {
			return 1;
		}
	}
	return 0;
}

int renderbuffer_add(struct renderbuffer *rb, struct sprite *s) {
	if ((s->flag & SPRITE_FLAG_INVISIBLE) == 0) {
		return rb_drawsprite(rb, s, 0);
	}
	return 0;
}

#ifdef PIXEL_LUA

#include "lua.h"
#include "lauxlib.h"

#define SRT_X 1
#define SRT_Y 2
#define SRT_SX 3
#define SRT_SY 4
#define SRT_ROT 5
#define SRT_SCALE 6

static double readkey(lua_State *L, int idx, int key, double def) {
	double ret;
	lua_pushvalue(L, lua_upvalueindex(key));
	lua_rawget(L, idx);
	ret = luaL_optnumber(L, -1, def);
	lua_pop(L, 1);
	return ret;
}

static struct srt *fill_srt(lua_State *L, struct srt *srt, int idx) {
	double x, y, scale, sx, sy, rot;
	if (lua_isnoneornil(L, idx)) {
		srt->offx = 0;
		srt->offy = 0;
		srt->rot = 0;
		srt->scalex = 1024;
		srt->scaley = 1024;
		return srt;
	}
	luaL_checktype(L, idx, LUA_TTABLE);
	x = readkey(L, idx, SRT_X, 0);
	y = readkey(L, idx, SRT_Y, 0);
	scale = readkey(L, idx, SRT_SCALE, 0);
	rot = readkey(L, idx, SRT_ROT, 0);
	if (scale > 0) {
		sx = sy = scale;
	} else {
		sx = readkey(L, idx, SRT_SX, 1.0);
		sy = readkey(L, idx, SRT_SY, 1.0);
	}
	srt->offx = (int)(x * SCREEN_SCALE);
	srt->offy = (int)(y * SCREEN_SCALE);
	srt->scalex = (int)(sx * 1024);
	srt->scaley = (int)(sy * 1024);
	srt->rot = (int)(rot * (1024.0 / 360.0));
	return srt;
}

static void lget_reftable(lua_State *L, int idx) {
	lua_getuservalue(L, idx);
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);
		lua_createtable(L, 0, 1);
		lua_pushvalue(L, -1);
		lua_setuservalue(L, idx);
	}
}

static int lps(lua_State *L) {
	struct sprite *s = (struct sprite *)lua_touserdata(L, 1);
	struct matrix *mat = &s->mat;
	int *m, n;
	int x, y, scale;
	if (!s->t.mat) {
		matrix_identity(mat);
		s->t.mat = mat;
	}
	m = mat->m;
	n = lua_gettop(L);
	switch (n) {
	case 4:
		x = (int)(luaL_checknumber(L, 2) * SCREEN_SCALE);
		y = (int)(luaL_checknumber(L, 3) * SCREEN_SCALE);
		scale = (int)(luaL_checknumber(L, 4) * 1024);
		m[0] = scale;
		m[1] = 0;
		m[2] = 0;
		m[3] = scale;
		m[4] = x;
		m[5] = y;
		break;
	case 3:
		x = (int)(luaL_checknumber(L, 2) * SCREEN_SCALE);
		y = (int)(luaL_checknumber(L, 3) * SCREEN_SCALE);
		m[4] = x;
		m[5] = y;
		break;
	case 2:
		scale = (int)(luaL_checknumber(L, 2) * 1024);
		m[0] = scale;
		m[1] = 0;
		m[2] = 0;
		m[3] = scale;
		break;
	default:
		return luaL_error(L, "invalid param for ps");
	}
	return 0;
}

static int lsr(lua_State *L) {
	struct sprite *s = (struct sprite *)lua_touserdata(L, 1);
	struct matrix *mat = &s->mat;
	int n;
	int sx = 1024, sy = 1024, rot = 0;
	if (!s->t.mat) {
		matrix_identity(mat);
		s->t.mat = mat;
	}
	n = lua_gettop(L);
	switch (n) {
	case 4:
		rot = (int)(luaL_checknumber(L, 4) * 1024.0 / 360.0);
	case 3:
		sx = (int)(luaL_checknumber(L, 2) * 1024);
		sy = (int)(luaL_checknumber(L, 3) * 1024);
		break;
	case 2:
		rot = (int)(luaL_checknumber(L, 2) * 1024.0 / 360.0);
		break;
	default:
		return luaL_error(L, "invalid param for sr");
	}
	matrix_sr(mat, sx, sy, rot);
	return 0;
}


static int ldraw(lua_State *L) {
	struct sprite *s = (struct sprite *)lua_touserdata(L, 1);
	struct srt srt;
	sprite_draw(s, fill_srt(L, &srt, 2));
	return 0;
}

static int lfetch(lua_State *L) {
	struct sprite *s = (struct sprite *)lua_touserdata(L, 1);
	const char *name = luaL_checkstring(L, 2);
	int idx = sprite_child_idx(s, name);
	if (idx < 0) {
		return 0;
	}
	if ((s->flag & SPRITE_FLAG_MULTIMOUNT) == 0) {
		lua_getuservalue(L, 1);
		lua_rawgeti(L, -1, idx + 1);
		if (!lua_isnil(L, -1)) {
			int top = lua_gettop(L);
			lget_reftable(L, top);
			lua_pushvalue(L, 1);
			lua_rawseti(L, -2, 0);
			lua_pop(L, 1);
		}
	}
	return 1;
}

static int lmount(lua_State *L) {
	struct sprite *c;
	struct sprite *s = (struct sprite *)lua_touserdata(L, 1);
	const char *name = luaL_checkstring(L, 2);
	int idx = sprite_child_idx(s, name);
	if (idx < 0) {
		return luaL_error(L, "can not find child :%s", name);
	}
	lua_getuservalue(L, 1);
	c = (struct sprite *)lua_touserdata(L, 3);
	lua_rawgeti(L, -1, idx + 1);
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);
	} else {
		struct sprite *oc = (struct sprite *)lua_touserdata(L, -1);
		if (oc == c) {
			return 0;
		}
		if ((oc->flag & SPRITE_FLAG_MULTIMOUNT) == 0) {
			lua_getuservalue(L, -1);
			if (lua_istable(L, -1)) {
				lua_pushnil(L);
				lua_rawseti(L, -2, 0);
			}
			lua_pop(L, 2);
		} else {
			lua_pop(L, 1);
		}
	}
	if (!c) {
		sprite_mount(s, idx, 0);
		lua_pushnil(L);
		lua_rawseti(L, -2, idx + 1);
	} else {
		if (c->parent) {
			int cidx;
			struct sprite *p;
			lua_getuservalue(L, 3);
			lua_rawgeti(L, -1, 0);
			p = (struct sprite *)lua_touserdata(L, -1);
			if (!p) {
				return luaL_error(L, "no parent");
			}
			cidx = sprite_child_idx1(p, c);
			if (idx < 0) {
				return luaL_error(L, "invalid child");
			}
			lua_getuservalue(L, -1);
			lua_pushnil(L);
			lua_rawseti(L, -2, cidx + 1);
			lua_pop(L, 3);
			sprite_mount(p, cidx, 0);
		}
		sprite_mount(s, idx, c);
		lua_pushvalue(L, 3);
		lua_rawseti(L, -2, idx + 1);
		if ((c->flag & SPRITE_FLAG_MULTIMOUNT) == 0) {
			lua_getuservalue(L, 3);
			if (lua_isnil(L, -1)) {
				lua_pop(L, 1);
				lua_createtable(L, 0, 1);
				lua_pushvalue(L, -1);
				lua_setuservalue(L, 3);
			}
			lua_pushvalue(L, 1);
			lua_rawseti(L, -2, 0);
			lua_rawseti(L, -2, 0);
		}
	}
	return 0;
}
static struct sprite *lookup(lua_State *L, struct sprite *spr) {
	int j;
	struct sprite *root = (struct sprite *)lua_touserdata(L, -1);
	lua_getuservalue(L, -1);
	for (j = 0; sprite_component(root, j) >= 0; j++) {
		struct sprite *cc = root->data.children[j];
		if (cc) {
			lua_rawgeti(L, -1, j + 1);
			if (cc == spr) {
				int cidx = lua_gettop(L);
				int pidx = cidx - 2;
				lget_reftable(L, cidx);
				lua_pushvalue(L, pidx);
				lua_rawseti(L, -2, 0);
				lua_pop(L, 1);
				lua_replace(L, -2);
				return cc;
			} else {
				lua_pop(L, 1);
			}
		}
	}
	lua_pop(L, 1);
	return 0;
}
static int unwind(lua_State *L, struct sprite *root, struct sprite *spr) {
	int depth = 0;
	while (spr) {
		if (spr == root) {
			return depth;
		}
		++depth;
		lua_checkstack(L, 3);
		lua_pushlightuserdata(L, spr);
		spr = spr->parent;
	}
	return -1;
}
static int ltest(lua_State *L) {
	int i, depth = 0;
	struct sprite *s = (struct sprite *)lua_touserdata(L, 1);
	int x = (int)luaL_checkinteger(L, 2);
	int y = (int)luaL_checkinteger(L, 3);
	struct srt srt;
	struct sprite *c = sprite_test(s, fill_srt(L, &srt, 4), x, y);
	if (!c) {
		return 0;
	}
	lua_settop(L, 1);
	depth = unwind(L, s, c);
	if (depth < 0) {
		return luaL_error(L, "invalid sprite in test");
	}
	lua_pushvalue(L, 1);
	for (i = depth + 1; i > 1; i--) {
		struct sprite *tmp = (struct sprite *)lua_touserdata(L, i);
		tmp = lookup(L, tmp);
		if (!tmp) {
			return luaL_error(L, "find an invalid sprite");
		}
		lua_replace(L, -2);
	}
	return 1;
}

static int laabb(lua_State *L) {
	int i;
	int aabb[4];
	struct sprite *s = (struct sprite *)lua_touserdata(L, 1);
	int world = lua_toboolean(L, 3);
	struct srt srt;
	sprite_aabb(s, fill_srt(L, &srt, 2), world, aabb);
	for (i = 0; i < 4; i++) {
		lua_pushinteger(L, aabb[i]);
	}
	return 4;
}

static int lchar_size(lua_State *L) {
	int i, idx = 0;
	const char *str = 0;
	struct sprite *s = (struct sprite *)lua_touserdata(L, 1);
	if (s->type != TYPE_LABEL) {
		return luaL_error(L, "char size need a label");
	}
	luaL_checktype(L, 2, LUA_TTABLE);
	lua_pushinteger(L, s->s.label->width);
	lua_rawseti(L, 2, ++idx);
	lua_pushinteger(L, s->s.label->height);
	lua_rawseti(L, 2, ++idx);
	if (!lua_isnil(L, 3)) {
		str = lua_tostring(L, 3);
	} else {
		if (!s->data.rich_text || !s->data.rich_text->text) {
			lua_pushinteger(L, idx);
			return 1;
		}
		str = s->data.rich_text->text;
	}
	if (!str) {
		lua_pushinteger(L, idx);
		return 1;
	}
	for (i = 0; str[i]; ) {
		int width = 0, height = 0, unicode = 0;
		int len = label_char_size(s->s.label, str + i, &width, &height, &unicode);
		lua_pushinteger(L, width);
		lua_rawseti(L, 2, ++idx);
		lua_pushinteger(L, height);
		lua_rawseti(L, 2, ++idx);
		lua_pushinteger(L, len);
		lua_rawseti(L, 2, ++idx);
		lua_pushinteger(L, unicode);
		lua_rawseti(L, 2, ++idx);
		i += len;
	}
	lua_pushinteger(L, idx);
	return 1;
}

static int lchildren_name(lua_State *L) {
	int i, cnt = 0;
	struct pack_animation *ani;
	struct sprite *s = (struct sprite *)lua_touserdata(L, 1);
	if (s->type != TYPE_ANIMATION) {
		return 0;
	}
	ani = s->s.ani;
	for (i = 0; i < ani->component_n; i++) {
		if (ani->component[i].name) {
			lua_pushstring(L, (const char *)ani->component[i].name);
			cnt++;
		}
	}
	return cnt;
}

static int lanchor_particle(lua_State *L) {
	struct sprite *p;
	struct sprite *s = (struct sprite *)lua_touserdata(L, 1);
	if (s->type != TYPE_ANCHOR) {
		return luaL_error(L, "need a anchor");
	}
	lget_reftable(L, 1);
	lua_pushvalue(L, 2);
	lua_rawseti(L, -2, 0);
	lua_pop(L, 1);
	s->data.anchor->ps = (struct particle_system *)lua_touserdata(L, 2);
	p = (struct sprite *)lua_touserdata(L, 3);
	if (!p || p->type != TYPE_PICTURE) {
		return luaL_error(L, "need a picture sprite");
	}
	s->data.anchor->pic = p->s.pic;
	return 0;
}

static int lchild_visible(lua_State *L) {
	struct sprite *s = (struct sprite *)lua_touserdata(L, 1);
	const char *name = luaL_checkstring(L, 2);
	lua_pushboolean(L, sprite_child_visible(s, name));
	return 1;
}

static const char *srt_key[] = {
	"x",
	"y",
	"sx",
	"sy",
	"rot",
	"scale",
};

static void lmethod(lua_State *L) {
	luaL_Reg l[] = {
		{"ps", lps},
		{"sr", lsr},
		{"draw", ldraw},
		{"fetch", lfetch},
		{"mount", lmount},
		{"test", ltest},
		{"aabb", laabb},
		{"char_size", lchar_size},
		{"child_visible", lchild_visible},
		{"children_name", lchildren_name},
		{"anchor_particle", lanchor_particle},
		{0, 0},
	};

	int i, n;
	luaL_newlib(L, l);
	n = sizeof(srt_key) / sizeof(srt_key[0]);
	for (i = 0; i < n; i++) {
		lua_pushstring(L, srt_key[i]);
	}
	luaL_setfuncs(L, l, n);
}

static int lget_name(lua_State *L) {
	struct sprite *s = (struct sprite *)lua_touserdata(L, 1);
	if (!s->name) {
		return 0;
	}
	lua_pushstring(L, s->name);
	return 1;
}

static int lget_frame(lua_State *L) {
	struct sprite *s = (struct sprite *)lua_touserdata(L, 1);
	lua_pushinteger(L, s->frame);
	return 1;
}

static int lget_total_frame(lua_State *L) {
	struct sprite *s = (struct sprite *)lua_touserdata(L, 1);
	lua_pushinteger(L, s->total_frame);
	return 1;
}

static int lget_force_frame(lua_State *L) {
	struct sprite *s = (struct sprite *)lua_touserdata(L, 1);
	lua_pushboolean(L, s->flag & SPRITE_FLAG_FORCEFRAME);
	return 1;
}

static int lget_visible(lua_State *L) {
	struct sprite *s = (struct sprite *)lua_touserdata(L, 1);
	lua_pushboolean(L, (s->flag & SPRITE_FLAG_INVISIBLE) == 0);
	return 1;
}

static int lget_matrix(lua_State *L) {
	struct sprite *s = (struct sprite *)lua_touserdata(L, 1);
	if (!s->t.mat) {
		s->t.mat = &s->mat;
		matrix_identity(&s->mat);
	}
	lua_pushlightuserdata(L, s->t.mat);
	return 1;
}

static int lget_world_matrix(lua_State *L) {
	struct sprite *s = (struct sprite *)lua_touserdata(L, 1);
	if (s->type == TYPE_ANCHOR) {
		lua_pushlightuserdata(L, s->s.mat);
		return 1;
	}
	return luaL_error(L, "world matrix need a anchor");
}

static int lget_type(lua_State *L) {
	struct sprite *s = (struct sprite *)lua_touserdata(L, 1);
	lua_pushinteger(L, s->type);
	return 1;
}

static int lget_color(lua_State *L) {
	struct sprite *s = (struct sprite *)lua_touserdata(L, 1);
	if (s->type == TYPE_LABEL) {

	} else {
		lua_pushinteger(L, s->t.color);
	}
	return 1;
}

static int lget_alpha(lua_State *L) {
	struct sprite *s = (struct sprite *)lua_touserdata(L, 1);
	lua_pushinteger(L, s->t.color >> 24);
	return 1;
}

static int lget_additive(lua_State *L) {
	struct sprite *s = (struct sprite *)lua_touserdata(L, 1);
	lua_pushinteger(L, s->t.addi);
	return 1;
}

static int lget_parent(lua_State *L) {
	struct sprite *s = (struct sprite *)lua_touserdata(L, 1);
	if (!s->parent) {
		return 0;
	}
	lua_getuservalue(L, 1);
	lua_rawgeti(L, -1, 0);
	return 1;
}

static int lget_program(lua_State *L) {
	struct sprite *s = (struct sprite *)lua_touserdata(L, 1);
	lua_pushinteger(L, s->t.pid);
	return 1;
}

static int lget_material(lua_State *L) {
	struct sprite *s = (struct sprite *)lua_touserdata(L, 1);
	if (!s->material) {
		return 0;
	}
	lget_reftable(L, 1);
	lua_getfield(L, -1, "material");
	return 1;
}

static int lget_text(lua_State *L) {
	struct sprite *s = (struct sprite *)lua_touserdata(L, 1);
	if (s->type != TYPE_LABEL) {
		return luaL_error(L, "text need a label");
	}
	if (s->data.rich_text) {
		lua_pushstring(L, s->data.rich_text->text);
		return 1;
	}
	return 0;
}

static int lget_message(lua_State *L) {
	struct sprite *s = (struct sprite *)lua_touserdata(L, 1);
	lua_pushboolean(L, s->flag & SPRITE_FLAG_MESSAGE);
	return 1;
}

static uint32_t ud_key = 0xffff;
static int lget_ud(lua_State *L) {
	lget_reftable(L, 1);
	lua_rawgetp(L, -1, &ud_key);
	if (lua_isnoneornil(L, -1)) {
		lua_newtable(L);
		lua_pushvalue(L, -1);
		lua_rawsetp(L, 2, &ud_key);
	}
	return 1;
}

static void lgetter(lua_State *L) {
	luaL_Reg l[] = {
		{"name", lget_name},
		{"frame", lget_frame},
		{"total_frame", lget_total_frame},
		{"visible", lget_visible},
		{"force_frame", lget_force_frame},
		{"matrix", lget_matrix},
		{"world_matrix", lget_world_matrix},
		{"type", lget_type},
		{"color", lget_color},
		{"alpha", lget_alpha},
		{"additive", lget_additive},
		{"parent", lget_parent},
		{"program", lget_program},
		{"material", lget_material},
		{"text", lget_text},
		{"message", lget_message},
		{"ud", lget_ud},
		{0, 0},
	};
	luaL_newlib(L, l);
}

static int lset_frame(lua_State *L) {
	struct sprite *s = (struct sprite *)lua_touserdata(L, 1);
	int frame = (int)luaL_checkinteger(L, 2);
	sprite_frame(s, frame, 0);
	return 0;
}

static int lset_action(lua_State *L) {
	struct sprite *s = (struct sprite *)lua_touserdata(L, 1);
	const char *action = lua_tostring(L, 2);
	sprite_action(s, action);
	return 0;
}

static int lset_force_frame(lua_State *L) {
	struct sprite *s = (struct sprite *)lua_touserdata(L, 1);
	if (lua_toboolean(L, 2))
		s->flag |= SPRITE_FLAG_FORCEFRAME;
	else
		s->flag &= ~SPRITE_FLAG_FORCEFRAME;
	return 0;
}

static int lset_visible(lua_State *L) {
	struct sprite *s = (struct sprite *)lua_touserdata(L, 1);
	if (lua_toboolean(L, 2))
		s->flag &= ~SPRITE_FLAG_INVISIBLE;
	else
		s->flag |= SPRITE_FLAG_INVISIBLE;
	return 0;
}

static int lset_matrix(lua_State *L) {
	struct sprite *s = (struct sprite *)lua_touserdata(L, 1);
	struct matrix *mat = (struct matrix *)lua_touserdata(L, 2);
	if (!mat) {
		return luaL_error(L, "invalid matrix");
	}
	s->t.mat = &s->mat;
	s->mat = *mat;
	return 0;
}

static int lset_color(lua_State *L) {
	struct sprite *s = (struct sprite *)lua_touserdata(L, 1);
	uint32_t color = (uint32_t)luaL_checkinteger(L, 2);
	s->t.color = color;
	return 0;
}

static int lset_alpha(lua_State *L) {
	struct sprite *s = (struct sprite *)lua_touserdata(L, 1);
	uint8_t alpha = (uint8_t)lua_tonumber(L, 2);
	s->t.color = (s->t.color & 0xffffff) | (alpha << 24);
	return 0;
}

static int lset_additive(lua_State *L) {
	struct sprite *s = (struct sprite *)lua_touserdata(L, 1);
	uint32_t additive = (uint32_t)luaL_checkinteger(L, 2);
	s->t.addi = additive;
	return 0;
}

static int lset_program(lua_State *L) {
	struct sprite *s = (struct sprite *)lua_touserdata(L, 1);
	if (lua_isnoneornil(L, 2)) {
		s->t.pid = PROGRAM_DEFAULT;
	} else {
		s->t.pid = (int)luaL_checkinteger(L, 2);
	}
	if (s->material) {
		s->material = 0;
		lget_reftable(L, 1);
		lua_pushnil(L);
		lua_setfield(L, -2, "material");
	}
	return 0;
}

static int lset_scissor(lua_State *L) {
	struct sprite *s = (struct sprite *)lua_touserdata(L, 1);
	if (s->type != TYPE_PANEL) {
		return luaL_error(L, "scissor need a panel");
	}
	s->data.scissor = lua_toboolean(L, 2);
	return 0;
}

static int lset_text(lua_State *L) {
	struct sprite *s = (struct sprite *)lua_touserdata(L, 1);
	if (s->type != TYPE_LABEL) {
		return luaL_error(L, "set text need a label");
	}
	if (lua_isnoneornil(L, 2)) {
		s->data.rich_text = 0;
		lget_reftable(L, 1);
		lua_pushnil(L);
		lua_setfield(L, -2, "richtext");
		return 0;
	}
	s->data.rich_text = 0;
	if (!lua_istable(L, 2) || lua_rawlen(L, 2) != 4) {
		return luaL_error(L, "rich text need a table");
	} else {
		const char *text;
		int i, n, size;
		struct label_field *fields;
		struct rich_text *rich;
		lua_rawgeti(L, 2, 1);
		text = luaL_checkstring(L, -1);
		lua_pop(L, 1);
		lua_rawgeti(L, 2, 2);
		n = lua_rawlen(L, -1);
		lua_pop(L, 1);
		rich = (struct rich_text *)lua_newuserdata(L, sizeof *rich);
		rich->text = text;
		rich->count = n;
		lua_rawgeti(L, 2, 3);
		rich->width = (int)luaL_checkinteger(L, -1);
		lua_pop(L, 1);
		lua_rawgeti(L, 2, 4);
		rich->height = (int)luaL_checkinteger(L, -1);
		lua_pop(L, 1);
		size = n * sizeof(struct label_field);
		rich->fields = (struct label_field *)lua_newuserdata(L, size);
		fields = rich->fields;
		lua_rawgeti(L, 2, 2);
		for (i = 0; i < n; i++) {
			uint32_t type;

			lua_rawgeti(L, -1, i + 1);
			if (!lua_istable(L, -1)) {
				return luaL_error(L, "rich need a table");
			}
			lua_rawgeti(L, -1, 1);
			((struct label_field *)(fields + i))->start = (uint32_t)luaL_checkinteger(L, -1);
			lua_pop(L, 1);

			lua_rawgeti(L, -1, 2);
			((struct label_field *)(fields + i))->end = (uint32_t)luaL_checkinteger(L, -1);
			lua_pop(L, 1);

			lua_rawgeti(L, -1, 3);
			type = (uint32_t)luaL_checkinteger(L, -1);
			((struct label_field *)(fields + i))->type = type;
			lua_pop(L, 1);

			lua_rawgeti(L, -1, 4);
			if (type == RL_COLOR) {
				((struct label_field *)(fields + i))->color = (uint32_t)luaL_checkinteger(L, -1);
			} else {
				((struct label_field *)(fields + i))->val = (uint32_t)luaL_checkinteger(L, -1);
			}
			lua_pop(L, 1);

			lua_pop(L, 1);
		}
		lua_pop(L, 1);
		lget_reftable(L, 1);
		lua_createtable(L, 3, 0);
		lua_pushvalue(L, 3);
		lua_rawseti(L, -2, 1);

		lua_pushvalue(L, 4);
		lua_rawseti(L, -2, 2);

		lua_rawgeti(L, 2, 1);
		lua_rawseti(L, -2, 3);

		lua_setfield(L, -2, "richtext");
		s->data.rich_text = rich;
	}
	return 0;
}

static int lset_message(lua_State *L) {
	struct sprite *s = (struct sprite *)lua_touserdata(L, 1);
	if (lua_toboolean(L, 2))
		s->flag |= SPRITE_FLAG_MESSAGE;
	else
		s->flag &= ~SPRITE_FLAG_MESSAGE;
	return 0;
}

static void lsetter(lua_State *L) {
	luaL_Reg l[] = {
		{"frame", lset_frame},
		{"action", lset_action},
		{"visible", lset_visible},
		{"force_frame", lset_force_frame},
		{"matrix", lset_matrix},
		{"color", lset_color},
		{"alpha", lset_alpha},
		{"additive", lset_additive},
		{"program", lset_program},
		{"scissor", lset_scissor},
		{"text", lset_text},
		{"message", lset_message},
		{0, 0},
	};
	luaL_newlib(L, l);
}

static struct sprite *l_new(lua_State *L, struct sprite_pack *pack, int id) {
	int i;
	int size;
	struct sprite *s;
	if (id == ANCHOR_ID) {
		size = sizeof(struct sprite) + sizeof(struct anchor_data);
		s = (struct sprite *)lua_newuserdata(L, size);
		return _anchor_new(s, pack, id);
	}
	if (pack->type[id] == TYPE_ANIMATION) {
		struct pack_animation *ani = (struct pack_animation *)pack->data[id];
		size = sizeof(struct sprite) + (ani->component_n - 1)*sizeof(struct sprite *);
	} else {
		size = sizeof(struct sprite);
	}
	s = (struct sprite *)lua_newuserdata(L, size);
	s->parent = 0;
	s->t.mat = 0;
	s->t.color = 0xffffffff;
	s->t.addi = 0;
	s->t.pid = PROGRAM_DEFAULT;
	s->flag = 0;
	s->name = 0;
	s->id = id;
	s->type = pack->type[id];
	s->material = 0;
	if (pack->type[id] == TYPE_ANIMATION) {
		s->s.ani = (struct pack_animation *)pack->data[id];
		s->frame = 0;
		sprite_action(s, 0);
		for (i = 0; i < s->s.ani->component_n; i++) {
			s->data.children[i] = 0;
		}
	} else {
		s->s.pic = (struct pack_picture *)pack->data[id];
		s->start_frame = 0;
		s->total_frame = 0;
		s->frame = 0;
		memset(&s->data, 0, sizeof(s->data));
		if (s->type == TYPE_PANEL) {
			struct pack_panel *pp = (struct pack_panel *)pack->data[id];
			s->data.scissor = pp->scissor;
		}
	}
	for (i = 0; ; i++) {
		struct sprite *cs;
		int cid = sprite_component(s, i);
		if (cid < 0) {
			break;
		}
		if (i == 0) {
			lua_newtable(L);
			lua_pushvalue(L, -1);
			lua_setuservalue(L, -3);
		}
		cs = l_new(L, pack, cid);
		if (cs) {
			cs->name = sprite_childname(s, i);
			sprite_mount(s, i, cs);
			update_message(pack, cs, id, i, s->frame);
			lua_rawseti(L, -2, i + 1);
		}
	}
	if (i > 0) {
		lua_pop(L, 1);
	}
	return s;
}

static int lnew(lua_State *L) {
	struct sprite_pack *pack = (struct sprite_pack *)lua_touserdata(L, 1);
	int id = (int)lua_tointeger(L, 2);
	l_new(L, pack, id);
	return 1;
}

static int lmaterial(lua_State *L) {
	struct sprite *s = (struct sprite *)lua_touserdata(L, 1);
	int size = material_size(s->t.pid);
	if (size == 0) {
		return luaL_error(L, "program has no material");
	}
	lget_reftable(L, 1);
	lua_createtable(L, 0, 1);
	s->material = (struct material *)lua_newuserdata(L, size);
	memset(s->material, 0, size);
	material_init(s->material, s->t.pid);
	lua_setfield(L, -2, "__obj");
	lua_pushvalue(L, -1);
	lua_setfield(L, -3, "material");
	lua_pushinteger(L, s->t.pid);
	return 2;
}

static int llabel(lua_State *L) {
	struct pack_label label;
	const char *align;
	int size;
	label.width = (int)luaL_checkinteger(L, 1);
	label.height = (int)luaL_checkinteger(L, 2);
	label.size = (int)luaL_checkinteger(L, 3);
	label.color = (uint32_t)luaL_optinteger(L, 4, 0xffffffff);
	align = lua_tostring(L, 5);
	label.space_w = (int)lua_tointeger(L, 6);
	label.space_h = (int)lua_tointeger(L, 7);
	label.auto_scale = (int)lua_tointeger(L, 8);
	label.edge = (int)lua_tointeger(L, 9);
	if (!align) {
		label.align = LABEL_ALIGN_RIGHT;
	} else {
		switch (align[0]) {
		case 'l':
		case 'L':
			label.align = LABEL_ALIGN_LEFT;
			break;
		case 'r':
		case 'R':
			label.align = LABEL_ALIGN_RIGHT;
			break;
		case 'c':
		case 'C':
			label.align = LABEL_ALIGN_CENTER;
			break;
		default:
			return luaL_error(L, "align need left/right/center");
		}
	}
	size = sizeof(struct sprite) + sizeof(struct pack_label);
	sprite_label_init((struct sprite *)lua_newuserdata(L, size), &label);
	return 1;
}

static int lproxy(lua_State *L) {
	static struct pack_part part = {
		{
			NULL,		// mat
				0xffffffff,	// color
				0,			// additive
				PROGRAM_DEFAULT,
		},	// trans
		0,	// component_id
		0,	// touchable
	};
	static struct pack_frame frame = {
		&part,
		1,	// n
	};
	static struct pack_action action = {
		0,	// name
		1,	// number
		0,	// start_frame
	};
	static struct pack_animation ani = {
		&frame,
		&action,
		1,	// frame_number
		1,	// action_number
		1,	// component_number
		{{
			(uint8_t *)"proxy",	// name
				0,		// id
		}},
	};
	struct sprite *s = (struct sprite *)lua_newuserdata(L, sizeof(struct sprite));
	lua_newtable(L);
	lua_setuservalue(L, -2);
	s->parent = 0;
	s->s.ani = &ani;
	s->t.mat = 0;
	s->t.color = 0xffffffff;
	s->t.addi = 0;
	s->t.pid = PROGRAM_DEFAULT;
	s->flag = SPRITE_FLAG_MULTIMOUNT;
	s->name = 0;
	s->id = 0;
	s->type = TYPE_ANIMATION;
	s->start_frame = 0;
	s->total_frame = 0;
	s->frame = 0;
	s->material = 0;
	s->data.children[0] = 0;
	sprite_action(s, 0);
	return 1;
}

int pixel_sprite(lua_State *L) {
	luaL_Reg l[] = {
		{"new", lnew},
		{"material", lmaterial},
		{"label", llabel},
		{"proxy", lproxy},
		{0, 0},
	};
	luaL_newlib(L, l);
	lmethod(L);
	lua_setfield(L, -2, "method");
	lgetter(L);
	lua_setfield(L, -2, "get");
	lsetter(L);
	lua_setfield(L, -2, "set");
	return 1;
}

#endif // PIXEL_LUA
