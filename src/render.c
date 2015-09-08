#include "render.h"
#include "opengl.h"
#include "array.h"
#include "pixel.h"

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MAX_ATTRIB 16
#define MAX_VB_SLOT 8
#define MAX_TEXTURE 8

#define CHANGE_INDEXBUFFER 0x1
#define CHANGE_VERTEXBUFFER 0x2
#define CHANGE_TEXTURE 0x4
#define CHANGE_BLEND 0x8
#define CHANGE_DEPTH 0x10
#define CHANGE_CULL 0x20
#define CHANGE_TARGET 0x40
#define CHANGE_SCISSOR 0x80


#define CHECK_GL_ERROR() \
	do { \
		GLenum error = glGetError(); \
		if (error != GL_NO_ERROR) { \
			pixel_log("GL_ERROR (0x%x) @ %s : %d\n", error, __FILE__, __LINE__); \
		} \
	} while(0);

struct attrib {
	int n;
	struct vertex_attrib a[MAX_ATTRIB];
};

struct attrib_layout {
	int vbslot;
	GLint size;
	GLenum type;
	GLboolean normalized;
	int offset;
};

struct buffer {
	GLuint glid;
	GLenum gltype;
	int n;
	int stride;
};

struct texture {
	GLuint glid;
	int width;
	int height;
	enum TEXTURE_FORMAT format;
	enum TEXTURE_TYPE type;
	int mipmap;
	int memsize;
};

struct target {
	GLuint glid;
	int tid;
};

struct shader {
	GLuint glid;
	int n;
	struct attrib_layout a[MAX_ATTRIB];
	int texture_n;
	int texture_uniform[MAX_TEXTURE];
};

struct state {
	int indexbuffer;
	int target;
	enum BLEND_FORMAT blend_src;
	enum BLEND_FORMAT blend_dst;
	enum DEPTH_FORMAT depth;
	enum CULL_MODE cull;
	int depthmask;
	int scissor;
	int texture[MAX_TEXTURE];
};

struct render {
	uint32_t changeflag;
	int attrib_layout;
	int vbslot[MAX_VB_SLOT];
	int pid;
	GLint framebuffer;
	struct state cur;
	struct state lst;
	struct array *buffer;
	struct array *attrib;
	struct array *target;
	struct array *texture;
	struct array *shader;
};

static struct render *R = 0;

int render_version(void) {
	return OPENGLES;
}

void render_init(struct render_arg *arg) {
	struct block b;
	void *data;
	int size;

	size = sizeof(struct render) +
		array_size(arg->max_buffer, sizeof(struct buffer)) +
		array_size(arg->max_layout, sizeof(struct attrib)) +
		array_size(arg->max_target, sizeof(struct target)) +
		array_size(arg->max_texture, sizeof(struct texture)) +
		array_size(arg->max_shader, sizeof(struct shader));

	data = malloc(size);

	block_init(&b, data, size);
	R = (struct render *)block_slice(&b, sizeof(struct render));
	memset(R, 0, sizeof(struct render));
	R->buffer = array_new(&b, arg->max_buffer, sizeof(struct buffer));
	R->attrib = array_new(&b, arg->max_layout, sizeof(struct attrib));
	R->target = array_new(&b, arg->max_target, sizeof(struct target));
	R->texture = array_new(&b, arg->max_texture, sizeof(struct texture));
	R->shader = array_new(&b, arg->max_shader, sizeof(struct shader));

	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &R->framebuffer);

	CHECK_GL_ERROR()
}

static void free_buffer(void *p, void *ud) {
	struct buffer *b = (struct buffer *)p;
	glDeleteBuffers(1, &b->glid);
	CHECK_GL_ERROR()
}

static void free_shader(void *p, void *ud) {
	struct shader *s = (struct shader *)p;
	glDeleteProgram(s->glid);
	CHECK_GL_ERROR()
}

static void free_texture(void *p, void *ud) {
	struct texture *t = (struct texture *)p;
	glDeleteTextures(1, &t->glid);
	CHECK_GL_ERROR()
}

static void free_target(void *p, void *ud) {
	struct target *t = (struct target *)p;
	glDeleteFramebuffers(1, &t->glid);
	CHECK_GL_ERROR()
}

void render_unit(void) {
	array_free(R->buffer, free_buffer, 0);
	array_free(R->shader, free_shader, 0);
	array_free(R->texture, free_texture, 0);
	array_free(R->target, free_target, 0);
	free(R);
	R = 0;
}

void render_set(enum RENDER_OBJ what, int id, int slot) {
	switch (what) {
	case VERTEXBUFFER:
		assert(slot >= 0 && slot < MAX_VB_SLOT);
		R->vbslot[slot] = id;
		R->changeflag |= CHANGE_VERTEXBUFFER;
		break;
	case INDEXBUFFER:
		R->cur.indexbuffer = id;
		R->changeflag |= CHANGE_INDEXBUFFER;
		break;
	case VERTEXLAYOUT:
		R->attrib_layout = id;
		break;
	case TEXTURE:
		assert(slot >= 0 && slot < MAX_TEXTURE);
		R->cur.texture[slot] = id;
		R->changeflag |= CHANGE_TEXTURE;
		break;
	case TARGET:
		R->cur.target = id;
		R->changeflag |= CHANGE_TARGET;
		break;
	default:
		assert(0);
		break;
	}
}

void render_rem(enum RENDER_OBJ what, int id) {
	if (!R) return;
	switch (what) {
	case VERTEXBUFFER:
	case INDEXBUFFER:
	{
		struct buffer *b = (struct buffer *)array_ref(R->buffer, id);
		if (b) {
			free_buffer(b, 0);
			array_rem(R->buffer, b);
		}
		break;
	}
	case SHADER:
	{
		struct shader *s = (struct shader *)array_ref(R->shader, id);
		if (s) {
			free_shader(s, 0);
			array_rem(R->shader, s);
		}
		break;
	}
	case TEXTURE:
	{
		struct texture *t = (struct texture *)array_ref(R->texture, id);
		if (t) {
			free_texture(t, 0);
			array_rem(R->texture, t);
		}
		break;
	}
	case TARGET:
	{
		struct target *t = (struct target *)array_ref(R->target, id);
		if (t) {
			free_target(t, 0);
			array_rem(R->target, t);
		}
		break;
	}
	default:
		assert(0);
		break;
	}
}

int render_vertexlayout(int n, struct vertex_attrib *attrib) {
	struct attrib *a;
	assert(n <= MAX_ATTRIB);

	a = (struct attrib *)array_add(R->attrib);
	if (!a) {
		return 0;
	}
	a->n = n;
	memcpy(a->a, attrib, n * sizeof(struct vertex_attrib));
	R->attrib_layout = array_id(R->attrib, a);
	return R->attrib_layout;
}

void render_setviewport(int x, int y, int width, int height) {
	glViewport(x, y, width, height);
}

void render_setscissor(int x, int y, int width, int height) {
	glScissor(x, y, width, height);
}

int render_buffer_create(enum RENDER_OBJ what, const void *data, int n, int stride) {
	GLenum gltype;
	struct buffer *b;

	switch (what) {
	case VERTEXBUFFER:
		gltype = GL_ARRAY_BUFFER;
		break;
	case INDEXBUFFER:
		gltype = GL_ELEMENT_ARRAY_BUFFER;
		break;
	default:
		return 0;
	}
	b = (struct buffer *)array_add(R->buffer);
	if (!b) {
		return 0;
	}
	glGenBuffers(1, &b->glid);
	glBindBuffer(gltype, b->glid);
	if (data && n > 0) {
		glBufferData(gltype, n*stride, data, GL_STATIC_DRAW);
		b->n = n;
	} else {
		b->n = 0;
	}
	b->gltype = gltype;
	b->stride = stride;

	CHECK_GL_ERROR()
		return array_id(R->buffer, b);
}

void render_buffer_update(int id, const void *data, int n) {
	struct buffer *b = (struct buffer *)array_ref(R->buffer, id);
	glBindBuffer(b->gltype, b->glid);
	glBufferData(b->gltype, n*b->stride, data, GL_DYNAMIC_DRAW);
	b->n = n;
	CHECK_GL_ERROR()
}

static int texture_size(enum TEXTURE_FORMAT fmt, int width, int height) {
	switch (fmt) {
	case TEXTURE_RGBA8:
		return width * height * 4;
	case TEXTURE_RGB565:
	case TEXTURE_RGBA4:
		return width * height * 2;
	case TEXTURE_RGB:
		return width * height * 3;
	case TEXTURE_A8:
	case TEXTURE_DEPTH:
		return width * height;
	case TEXTURE_PVR2:
		return width * height / 4;
	case TEXTURE_PVR4:
	case TEXTURE_ETC1:
		return width * height / 2;
	default:
		return 0;
	}
}

static int texture_fmt(struct texture *t, GLint *fmt, GLenum *type) {
	int compressed = 0;
	switch (t->format) {
	case TEXTURE_RGBA8:
		*fmt = GL_RGBA;
		*type = GL_UNSIGNED_BYTE;
		break;
	case TEXTURE_RGB:
		*fmt = GL_RGB;
		*type = GL_UNSIGNED_BYTE;
		break;
	case TEXTURE_RGBA4:
		*fmt = GL_RGBA;
		*type = GL_UNSIGNED_SHORT_4_4_4_4;
		break;
	case TEXTURE_RGB565:
		*fmt = GL_RGB;
		*type = GL_UNSIGNED_SHORT_5_6_5;
		break;
	case TEXTURE_A8:
	case TEXTURE_DEPTH:
		*fmt = GL_ALPHA;
		*type = GL_UNSIGNED_BYTE;
		break;
	default:
		assert(0);
		return -1;
	}
	return compressed;
}

int render_texture_create(int width, int height, enum TEXTURE_FORMAT fmt, enum TEXTURE_TYPE type, int mipmap) {
	int size;
	struct texture *t = (struct texture *)array_add(R->texture);
	if (!t) {
		return 0;
	}
	glGenTextures(1, &t->glid);
	t->width = width;
	t->height = height;
	t->format = fmt;
	t->type = type;
	assert(type == TEXTURE_2D || type == TEXTURE_CUBE);
	t->mipmap = mipmap;
	size = texture_size(fmt, width, height);
	if (mipmap) {
		size += size / 3;
	}
	if (type == TEXTURE_CUBE) {
		size *= 6;
	}
	t->memsize = size;
	CHECK_GL_ERROR()
		return array_id(R->texture, t);
}

void render_texture_update(int id, int width, int height, void *pixels, int slice, int miplevel) {
	GLenum gltype;
	GLint fmt;
	GLenum itype;
	int target;
	struct texture *t = (struct texture *)array_ref(R->texture, id);
	if (!t) {
		return;
	}
	if (t->type == TEXTURE_2D) {
		gltype = GL_TEXTURE_2D;
		target = GL_TEXTURE_2D;
	} else {
		assert(t->type == TEXTURE_CUBE);
		gltype = GL_TEXTURE_CUBE_MAP;
		target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + slice;
	}
	glActiveTexture(GL_TEXTURE7);
	R->changeflag |= CHANGE_TEXTURE;
	R->lst.texture[7] = 0;
	glBindTexture(gltype, t->glid);

	if (t->mipmap) {
		glTexParameteri(gltype, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	} else {
		glTexParameteri(gltype, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
	glTexParameteri(gltype, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(gltype, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(gltype, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	if (texture_fmt(t, &fmt, &itype)) {
		int size = texture_size(t->format, width, height);
		glCompressedTexImage2D(target, miplevel, fmt, (GLsizei)t->width, (GLsizei)t->height, 0, size, pixels);
	} else {
		glTexImage2D(target, miplevel, fmt, (GLsizei)width, (GLsizei)height, 0, fmt, itype, pixels);
	}
	CHECK_GL_ERROR()
}

void render_texture_subupdate(int id, const void *pixels, int x, int y, int w, int h) {
	struct texture *t;
	GLenum type;
	GLint fmt;
	int target;

	t = (struct texture *)array_ref(R->texture, id);
	if (!t) {
		return;
	}
	if (t->type == TEXTURE_2D) {
		type = GL_TEXTURE_2D;
		target = GL_TEXTURE_2D;
	} else {
		assert(t->type == TEXTURE_CUBE);
		type = GL_TEXTURE_CUBE_MAP;
		target = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
	}
	glActiveTexture(GL_TEXTURE7);
	R->changeflag |= CHANGE_TEXTURE;
	R->lst.texture[7] = 0;
	glBindTexture(type, t->glid);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	if (texture_fmt(t, &fmt, &type)) {
		int size = texture_size(t->format, w, h);
		glCompressedTexSubImage2D(target, 0, x, y, w, h, fmt, size, pixels);
	} else {
		glTexSubImage2D(target, 0, x, y, w, h, fmt, type, pixels);
	}
	CHECK_GL_ERROR()
}

int render_target_create(int width, int height, enum TEXTURE_FORMAT fmt) {
	int ttid, tid;
	struct target *t;
	struct texture *tex;

	tid = render_texture_create(width, height, fmt, TEXTURE_2D, 0);
	if (tid == 0) {
		return 0;
	}
	render_texture_update(tid, width, height, 0, 0, 0);
	t = (struct target *)array_add(R->target);
	if (!t) {
		return 0;
	}
	t->tid = tid;
	tex = (struct texture *)array_ref(R->texture, tid);
	if (!tex) {
		return 0;
	}
	glGenFramebuffers(1, &t->glid);
	glBindFramebuffer(GL_FRAMEBUFFER, t->glid);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex->glid, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		free_target(t, 0);
		return 0;
	}
	CHECK_GL_ERROR()
		ttid = array_id(R->target, t);

	glBindFramebuffer(GL_FRAMEBUFFER, R->framebuffer);
	R->lst.target = 0;
	R->changeflag |= CHANGE_TARGET;
	if (ttid == 0) {
		render_rem(TEXTURE, tid);
	}
	CHECK_GL_ERROR()
		return ttid;
}

int render_target_texture(int id) {
	struct target *t = (struct target *)array_ref(R->target, id);
	if (t) {
		return t->tid;
	}
	return 0;
}

static GLuint compile(const char *source, int type) {
	GLint ok;
	GLuint glid;

	glid = glCreateShader(type);
	glShaderSource(glid, 1, &source, 0);
	glCompileShader(glid);

	glGetShaderiv(glid, GL_COMPILE_STATUS, &ok);
	if (ok == GL_FALSE) {
		char buff[1024];
		GLint len;
		glGetShaderInfoLog(glid, 1024, &len, buff);
		pixel_log("compile:%s\n", buff);
		glDeleteShader(glid);
		return 0;
	}
	CHECK_GL_ERROR()
		return glid;
}

static int link(GLuint glid) {
	GLint ok;
	glLinkProgram(glid);

	glGetProgramiv(glid, GL_LINK_STATUS, &ok);
	if (ok == GL_FALSE) {
		char buff[1024];
		GLint len;
		glGetProgramInfoLog(glid, 1024, &len, buff);
		pixel_log("link:%s\n", buff);
		return 0;
	}
	CHECK_GL_ERROR()
		return 1;
}

static int compile_link(struct shader *s, const char *vs, const char *fs) {
	GLuint fs_glid, vs_glid;
	struct attrib *a;
	int i;

	fs_glid = compile(fs, GL_FRAGMENT_SHADER);
	if (!fs_glid) {
		return 0;
	} else {
		glAttachShader(s->glid, fs_glid);
	}
	vs_glid = compile(vs, GL_VERTEX_SHADER);
	if (!vs_glid) {
		return 0;
	} else {
		glAttachShader(s->glid, vs_glid);
	}

	if (R->attrib_layout == 0) {
		return 0;
	}

	a = (struct attrib *)array_ref(R->attrib, R->attrib_layout);
	s->n = a->n;
	for (i = 0; i < a->n; i++) {
		struct vertex_attrib *va = &a->a[i];
		struct attrib_layout *al = &s->a[i];
		glBindAttribLocation(s->glid, i, va->name);
		al->vbslot = va->vbslot;
		al->offset = va->offset;
		al->size = va->n;
		switch (va->size) {
		case 1:
			al->type = GL_UNSIGNED_BYTE;
			al->normalized = GL_TRUE;
			break;
		case 2:
			al->type = GL_UNSIGNED_SHORT;
			al->normalized = GL_TRUE;
			break;
		case 4:
			al->type = GL_FLOAT;
			al->normalized = GL_FALSE;
			break;
		default:
			return 0;
		}
	}
	return link(s->glid);
}

int render_shader_create(struct shader_arg *arg) {
	int i;
	struct shader *s = (struct shader *)array_add(R->shader);
	if (!s) {
		return 0;
	}
	s->glid = glCreateProgram();
	if (!compile_link(s, arg->vs, arg->fs)) {
		glDeleteProgram(s->glid);
		array_rem(R->shader, s);
		return 0;
	}
	s->texture_n = arg->texture;
	for (i = 0; i < s->texture_n; i++) {
		s->texture_uniform[i] = glGetUniformLocation(s->glid, arg->texture_uniform[i]);
	}
	CHECK_GL_ERROR()
		return array_id(R->shader, s);
}

static void texture_uniform(struct shader *s) {
	int i;
	for (i = 0; i < s->texture_n; i++) {
		int loc = s->texture_uniform[i];
		if (loc >= 0) {
			glUniform1i(loc, i);
		}
	}
}

void render_shader_bind(int id) {
	struct shader *s = (struct shader *)array_ref(R->shader, id);
	R->pid = id;
	R->changeflag |= CHANGE_VERTEXBUFFER;
	if (s) {
		glUseProgram(s->glid);
		texture_uniform(s);
	} else {
		glUseProgram(0);
	}
	CHECK_GL_ERROR()
}

int render_shader_locuniform(const char *name) {
	struct shader *s = (struct shader *)array_ref(R->shader, R->pid);
	if (s) {
		int loc = glGetUniformLocation(s->glid, name);
		CHECK_GL_ERROR()
			return loc;
	}
	return -1;
}

void render_shader_uniform(int loc, enum UNIFORM_FORMAT fmt, const float *v) {
	switch (fmt) {
	case UNIFORM_FLOAT1:
		glUniform1f(loc, v[0]);
		break;
	case UNIFORM_FLOAT2:
		glUniform2f(loc, v[0], v[1]);
		break;
	case UNIFORM_FLOAT3:
		glUniform3f(loc, v[0], v[1], v[2]);
		break;
	case UNIFORM_FLOAT4:
		glUniform4f(loc, v[0], v[1], v[2], v[3]);
		break;
	case UNIFORM_FLOAT33:
		glUniformMatrix3fv(loc, 1, GL_FALSE, v);
		break;
	case UNIFORM_FLOAT44:
		glUniformMatrix4fv(loc, 1, GL_FALSE, v);
		break;
	default:
		assert(0);
		return;
	}
	CHECK_GL_ERROR()
}

void render_setblend(enum BLEND_FORMAT src, enum BLEND_FORMAT dst) {
	R->cur.blend_src = src;
	R->cur.blend_dst = dst;
	R->changeflag |= CHANGE_BLEND;
}

void render_setdepth(enum DEPTH_FORMAT d) {
	R->cur.depth = d;
	R->changeflag |= CHANGE_DEPTH;
}

void render_setcull(enum CULL_MODE c) {
	R->cur.cull = c;
	R->changeflag |= CHANGE_CULL;
}

void render_enable_depthmask(int enable) {
	R->cur.depthmask = enable;
	R->changeflag |= CHANGE_DEPTH;
}

void render_enable_scissor(int enable) {
	R->cur.scissor = enable;
	R->changeflag |= CHANGE_SCISSOR;
}

static void apply_vertexbuffer(void) {
	struct shader *s = (struct shader *)array_ref(R->shader, R->pid);
	if (s) {
		int i;
		int lst_vb = 0;
		int stride = 0;
		for (i = 0; i < s->n; i++) {
			struct attrib_layout *al = &s->a[i];
			int vb = R->vbslot[al->vbslot];
			if (lst_vb != vb) {
				struct buffer *b = (struct buffer *)array_ref(R->buffer, vb);
				if (!b) {
					continue;
				}
				glBindBuffer(GL_ARRAY_BUFFER, b->glid);
				lst_vb = vb;
				stride = b->stride;
			}
			glEnableVertexAttribArray(i);
			glVertexAttribPointer(i, al->size, al->type, al->normalized, stride, (const GLvoid *)(ptrdiff_t)(al->offset));
		}
	}
	CHECK_GL_ERROR()
}

static void render_state_commit(void) {
	if (R->changeflag & CHANGE_INDEXBUFFER) {
		int id = R->cur.indexbuffer;
		if (id != R->lst.indexbuffer) {
			struct buffer *b = (struct buffer *)array_ref(R->buffer, id);
			R->lst.indexbuffer = id;
			if (b) {
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, b->glid);
				CHECK_GL_ERROR()
			}
		}
	}

	if (R->changeflag & CHANGE_VERTEXBUFFER) {
		apply_vertexbuffer();
	}

	if (R->changeflag & CHANGE_TEXTURE) {
		int i;
		static GLenum mode[] = {
			GL_TEXTURE_2D,
			GL_TEXTURE_CUBE_MAP,
		};
		for (i = 0; i < MAX_TEXTURE; i++) {
			int id = R->cur.texture[i];
			int lstid = R->lst.texture[i];
			if (id != lstid) {
				struct texture *t = (struct texture *)array_ref(R->texture, id);
				R->lst.texture[i] = id;
				if (t) {
					glActiveTexture(GL_TEXTURE0 + i);
					glBindTexture(mode[t->type], t->glid);
				}
			}
		}
		CHECK_GL_ERROR()
	}

	if (R->changeflag & CHANGE_TARGET) {
		int tid = R->cur.target;
		if (R->lst.target != tid) {
			GLuint glid = R->framebuffer;
			if (tid != 0) {
				struct target *t = (struct target *)array_ref(R->target, tid);
				if (t) {
					glid = t->glid;
				} else {
					glid = 0;
				}
			}
			glBindFramebuffer(GL_FRAMEBUFFER, glid);
			R->lst.target = tid;
			CHECK_GL_ERROR()
		}
	}

	if (R->changeflag & CHANGE_BLEND) {
		if (R->lst.blend_src != R->cur.blend_src || R->lst.blend_dst != R->cur.blend_dst) {
			enum BLEND_FORMAT src = R->cur.blend_src;
			enum BLEND_FORMAT dst = R->cur.blend_dst;
			static GLenum blend[] = {
				0,
				GL_ZERO,
				GL_ONE,
				GL_SRC_COLOR,
				GL_ONE_MINUS_SRC_COLOR,
				GL_SRC_ALPHA,
				GL_ONE_MINUS_SRC_ALPHA,
				GL_DST_ALPHA,
				GL_ONE_MINUS_DST_ALPHA,
				GL_DST_COLOR,
				GL_ONE_MINUS_DST_COLOR,
				GL_SRC_ALPHA_SATURATE,
			};

			if (R->cur.blend_src == BLEND_DISABLE) {
				glDisable(GL_BLEND);
			} else if (R->lst.blend_src == BLEND_DISABLE) {
				glEnable(GL_BLEND);
			}
			glBlendFunc(blend[src], blend[dst]);
			R->lst.blend_src = src;
			R->lst.blend_dst = dst;
		}
	}

	if (R->changeflag & CHANGE_DEPTH) {
		if (R->lst.depth != R->cur.depth) {
			if (R->lst.depth == DEPTH_DISABLE) {
				glEnable(GL_DEPTH_TEST);
			}
			if (R->cur.depth == DEPTH_DISABLE) {
				glDisable(GL_DEPTH_TEST);
			} else {
				static GLenum depth[] = {
					0,
					GL_LEQUAL,
					GL_LESS,
					GL_EQUAL,
					GL_GREATER,
					GL_GEQUAL,
					GL_ALWAYS,
				};
				glDepthFunc(depth[R->cur.depth]);
			}
			R->lst.depth = R->cur.depth;
		}
		if (R->lst.depthmask != R->cur.depthmask) {
			glDepthMask(R->cur.depthmask ? GL_TRUE : GL_FALSE);
			R->lst.depthmask = R->cur.depthmask;
		}
	}

	if (R->changeflag & CHANGE_CULL) {
		if (R->lst.cull != R->cur.cull) {
			if (R->lst.cull == CULL_DISABLE) {
				glEnable(GL_CULL_FACE);
			}
			if (R->cur.cull == CULL_DISABLE) {
				glDisable(GL_CULL_FACE);
			} else {
				glCullFace(R->cur.cull == CULL_FRONT ? GL_FRONT : GL_BACK);
			}
			R->lst.cull = R->cur.cull;
		}
	}

	if (R->changeflag & CHANGE_SCISSOR) {
		if (R->lst.scissor != R->cur.scissor) {
			if (R->cur.scissor) {
				glEnable(GL_SCISSOR_TEST);
			} else {
				glDisable(GL_SCISSOR_TEST);
			}
			R->lst.scissor = R->cur.scissor;
		}
	}

	R->changeflag = 0;
	CHECK_GL_ERROR()
}

void render_state_reset(void) {
	R->changeflag = ~0;
	memset(&R->lst, 0, sizeof(R->lst));
	glDisable(GL_BLEND);
	glDisable(GL_SCISSOR_TEST);
	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glBindFramebuffer(GL_FRAMEBUFFER, R->framebuffer);
	CHECK_GL_ERROR()
}

void render_clear(enum CLEAR_MASK mask, unsigned long argb) {
	GLbitfield m = 0;
	if (mask & MASKC) {
		float a = ((argb >> 24) & 0xff) / 255.0f;
		float r = ((argb >> 16) & 0xff) / 255.0f;
		float g = ((argb >> 8) & 0xff) / 255.0f;
		float b = ((argb >> 0) & 0xff) / 255.0f;
		glClearColor(r, g, b, a);
		m |= GL_COLOR_BUFFER_BIT;
	}
	if (mask & MASKD) {
		m |= GL_DEPTH_BUFFER_BIT;
	}
	if (mask & MASKS) {
		m |= GL_STENCIL_BUFFER_BIT;
	}
	render_state_commit();
	glClear(m);
	CHECK_GL_ERROR()
}

void render_draw(enum DRAW_MODE mode, int idx, int n) {
	int id;
	struct buffer *b;
	render_state_commit();
	id = R->cur.indexbuffer;
	b = (struct buffer *)array_ref(R->buffer, id);
	if (b) {
		static int draw_mode[] = {
			GL_TRIANGLES,
			GL_LINES,
		};
		int offset = idx;
		GLenum type = GL_UNSIGNED_SHORT;
		if (b->stride == 1) {
			type = GL_UNSIGNED_BYTE;
		} else {
			offset *= sizeof(short);
		}
		glDrawElements(draw_mode[mode], n, type, (char *)0 + offset);
		CHECK_GL_ERROR()
	}
}
