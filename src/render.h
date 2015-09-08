#ifndef _RENDER_H_
#define _RENDER_H_

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

	enum RENDER_OBJ {
		INVALID = 0,
		VERTEXLAYOUT = 1,
		VERTEXBUFFER = 2,
		INDEXBUFFER = 3,
		TEXTURE = 4,
		TARGET = 5,
		SHADER = 6,
	};

	enum TEXTURE_TYPE {
		TEXTURE_2D = 0,
		TEXTURE_CUBE,
	};

	enum TEXTURE_FORMAT {
		TEXTURE_INVALID = 0,
		TEXTURE_RGBA8,
		TEXTURE_RGBA4,
		TEXTURE_RGB,
		TEXTURE_RGB565,
		TEXTURE_A8,
		TEXTURE_DEPTH,	// use for render target
		TEXTURE_PVR2,
		TEXTURE_PVR4,
		TEXTURE_ETC1,
	};

	enum BLEND_FORMAT {
		BLEND_DISABLE = 0,
		BLEND_ZERO,
		BLEND_ONE,
		BLEND_SRC_COLOR,
		BLEND_ONE_MINUS_SRC_COLOR,
		BLEND_SRC_ALPHA,
		BLEND_ONE_MINUS_SRC_ALPHA,
		BLEND_DST_ALPHA,
		BLEND_ONE_MINUS_DST_ALPHA,
		BLEND_DST_COLOR,
		BLEND_ONE_MINUS_DST_COLOR,
		BLEND_SRC_ALPHA_SATURATE,
	};

	enum DEPTH_FORMAT {
		DEPTH_DISABLE = 0,
		DEPTH_LESS_EQUAL,
		DEPTH_LESS,
		DEPTH_EQUAL,
		DEPTH_GREATER,
		DEPTH_GREATER_EQUAL,
		DEPTH_ALWAYS,
	};

	enum CLEAR_MASK {
		MASKC = 0x1,
		MASKD = 0x2,
		MASKS = 0x4,
	};

	enum UNIFORM_FORMAT {
		UNIFORM_INVALID = 0,
		UNIFORM_FLOAT1,
		UNIFORM_FLOAT2,
		UNIFORM_FLOAT3,
		UNIFORM_FLOAT4,
		UNIFORM_FLOAT33,
		UNIFORM_FLOAT44,
	};

	enum DRAW_MODE {
		DRAW_TRIANGLE = 0,
		DRAW_LINE,
	};

	enum CULL_MODE {
		CULL_DISABLE = 0,
		CULL_FRONT,
		CULL_BACK,
	};

	struct vertex_attrib {
		const char * name;
		int vbslot;
		int n;
		int size;
		int offset;
	};

	struct shader_arg {
		const char *vs;
		const char *fs;
		int texture;
		const char **texture_uniform;
	};

	struct render_arg {
		int max_buffer;
		int max_layout;
		int max_target;
		int max_texture;
		int max_shader;
	};

	int render_version(void);
	void render_init(struct render_arg *arg);
	void render_unit(void);
	void render_set(enum RENDER_OBJ what, int id, int slot);
	void render_rem(enum RENDER_OBJ what, int id);
	int render_vertexlayout(int n, struct vertex_attrib *attrib);

	void render_setviewport(int x, int y, int width, int height);
	void render_setscissor(int x, int y, int width, int height);

	int render_buffer_create(enum RENDER_OBJ what, const void *data, int n, int stride);
	void render_buffer_update(int id, const void *data, int n);

	int render_texture_create(int width, int height, enum TEXTURE_FORMAT fmt, enum TEXTURE_TYPE type, int mipmap);
	void render_texture_update(int id, int width, int height, void *pixels, int slice, int miplevel);
	void render_texture_subupdate(int id, const void *pixels, int x, int y, int w, int h);

	int render_target_create(int width, int height, enum TEXTURE_FORMAT fmt);
	int render_target_texture(int id);

	int render_shader_create(struct shader_arg *arg);
	void render_shader_bind(int id);
	int render_shader_locuniform(const char *name);
	void render_shader_uniform(int loc, enum UNIFORM_FORMAT fmt, const float *v);

	void render_setblend(enum BLEND_FORMAT src, enum BLEND_FORMAT dst);
	void render_setdepth(enum DEPTH_FORMAT d);
	void render_setcull(enum CULL_MODE c);
	void render_enable_depthmask(int enable);
	void render_enable_scissor(int enable);

	void render_state_reset(void);
	void render_clear(enum CLEAR_MASK mask, unsigned long argb);
	void render_draw(enum DRAW_MODE mode, int idx, int n);

#ifdef __cplusplus
};
#endif
#endif // _RENDER_H_