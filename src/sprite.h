#ifndef _SPRITE_H_
#define _SPRITE_H_

#include "matrix.h"
#include "spritepack.h"

#define SPRITE_FLAG_INVISIBLE 0x01
#define SPRITE_FLAG_MESSAGE 0x02
#define SPRITE_FLAG_MULTIMOUNT 0x04
#define SPRITE_FLAG_FORCEFRAME 0x08

#ifdef __cplusplus
extern "C" {
#endif

	struct sprite;
	struct sprite *sprite_new(const char *packname, const char *name);
	void sprite_free(struct sprite *s);
	void sprite_draw(struct sprite *s, struct srt *srt);
	void sprite_drawquad(struct pack_picture *pic, const struct srt *srt, const struct sprite_trans *arg);
	void sprite_drawpolygon(struct pack_polygon *poly, const struct srt *srt, const struct sprite_trans *arg);
	struct sprite *sprite_label(struct pack_label *pl, const char *text);
	struct sprite *sprite_panel(struct pack_panel *pp);
	void sprite_ps(struct sprite *s, int x, int y, float scale);
	void sprite_scale(struct sprite *s, float scale);
	void sprite_sr(struct sprite *s, float sx, float sy, float r);
	void sprite_rot(struct sprite *s, float r);
	void sprite_aabb(struct sprite *s, struct srt *srt, int world, int aabb[4]);
	struct sprite *sprite_test(struct sprite *s, struct srt *srt, int x, int y);
	int sprite_visible(struct sprite *s, int visible);
	const char *sprite_name(struct sprite *s);
	void sprite_pmatrix(struct sprite *s, struct matrix *mat);
	struct matrix *sprite_worldmatrix(struct sprite *s);
	struct matrix *sprite_matrix(struct sprite *s, struct matrix *mat);
	int sprite_action(struct sprite *s, const char *action);
	//return component id
	int sprite_component(struct sprite *s, int idx);
	const char *sprite_childname(struct sprite *s, int idx);
	int sprite_children_name(struct sprite *s, const char *names[]);
	struct sprite *sprite_child(struct sprite *s, const char *name);
	int sprite_child_idx(struct sprite *s, const char *name);
	int sprite_child_idx1(struct sprite *s, struct sprite *c);
	int sprite_child_visible(struct sprite *s, const char *name);
	void sprite_mount(struct sprite *s, int idx, struct sprite *c);
	//-1 return current frame, other set frame
	int sprite_frame(struct sprite *s, int frame, int force);
	//inc current frame
	int sprite_frame1(struct sprite *s, int dframe);
	int sprite_total_frame(struct sprite *s);
	//0 return current text, other set text
	const char *sprite_text(struct sprite *s, const char *text);
	int sprite_scissor(struct sprite *s, int scissor);

	struct particle_system;
	void sprite_particle(struct sprite *s, struct particle_system *ps, struct sprite *a);
	void sprite_draw_triangle(int tid, float p[6], uint32_t color, uint32_t addi);

#ifdef PIXEL_LUA
#include "lua.h"
	int pixel_sprite(lua_State *L);
#endif // PIXEL_LUA

#ifdef __cplusplus
};
#endif
#endif // _SPRITE_H_
