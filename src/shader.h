#ifndef _SHADER_H_
#define _SHADER_H_

#include <stdint.h>
#include "renderbuffer.h"
#include "render.h"

#define PROGRAM_DEFAULT -1
#define PROGRAM_PICTURE 0
#define PROGRAM_RENDERBUFFER 1
#define PROGRAM_TEXT 2
#define PROGRAM_TEXT_EDGE 3
#define PROGRAM_GRAY 4
#define PROGRAM_COLOR 5
#define PROGRAM_BLEND 6

#ifdef __cplusplus
extern "C" {
#endif

	struct material;
	int shader_version(void);
	void shader_init(void);
	void shader_unit(void);

	void shader_load(int pid, const char *fragment, const char *vertex, int texture, const char **texture_uniform);
	void shader_texture(int id, int channel);
	void shader_program(int pid, struct material *m);
	void shader_flush(void);
	void shader_clear(unsigned long argb);

	void shader_blend(int m1, int m2);
	void shader_default_blend(void);
	void shader_scissor(int enable);

	int shader_add_uniform(int pid, const char *name, enum UNIFORM_FORMAT t);
	void shader_set_uniform(int pid, int idx, enum UNIFORM_FORMAT t, float *v);
	int shader_uniform_size(enum UNIFORM_FORMAT t);

	void shader_drawvertex(const struct vertex_pack vp[4], uint32_t color, uint32_t addi);
	void shader_drawpolygon(int n, const struct vertex_pack *vp, uint32_t color, uint32_t addi);
	void shader_drawbuffer(struct renderbuffer *rb, float x, float y, float scale);
	void shader_draw(int tid, const float tcoord[8], const float scoord[8], uint32_t color, uint32_t addi);

#ifdef PIXEL_LUA
#include "lua.h"
	int pixel_shader(lua_State *L);
#endif // PIXEL_LUA

#ifdef __cplusplus
};
#endif
#endif // _SHADER_H_
