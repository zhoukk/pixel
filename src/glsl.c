#include "opengl.h"
#include "shader.h"

static const char *sprite_v =
PRECISION
"attribute vec4 position;"
"attribute vec2 texcoord;"
"attribute vec4 color;"
"attribute vec4 additive;"

"varying vec2 v_texcoord;"
"varying vec4 v_color;"
"varying vec4 v_additive;"

"void main() {"
"	gl_Position = position + vec4(-1.0,1.0,0,0);"
"	v_texcoord = texcoord;"
"	v_color = color;"
"	v_additive = additive;"
"}";

static const char *sprite_f =
PRECISION
"varying vec2 v_texcoord;"
"varying vec4 v_color;"
"varying vec4 v_additive;"
"uniform sampler2D texture0;"

"void main() {"
"	vec4 tmp = texture2D(texture0, v_texcoord);"
"	gl_FragColor.xyz = tmp.xyz * v_color.xyz;"
"	gl_FragColor.w = tmp.w;"
"	gl_FragColor *= v_color.w;"
"	gl_FragColor.xyz += v_additive.xyz * tmp.w;"
"}";

static const char *text_f =
PRECISION
"varying vec2 v_texcoord;"
"varying vec4 v_color;"
"varying vec4 v_additive;"

"uniform sampler2D texture0;"

"void main() {"
"	float c = texture2D(texture0, v_texcoord).w;"
"	float alpha = clamp(c, 0.0, 0.5) * 2.0;"

"	gl_FragColor.xyz = (v_color.xyz + v_additive.xyz) * alpha;"
"	gl_FragColor.w = alpha;"
"	gl_FragColor *= v_color.w;"
"}";

static const char *text_edge_f =
PRECISION
"varying vec2 v_texcoord;"
"varying vec4 v_color;"
"varying vec4 v_additive;"

"uniform sampler2D texture0;"

"void main() {"
"	float c = texture2D(texture0, v_texcoord).w;"
"	float alpha = clamp(c, 0.0, 0.5) * 2.0;"
"	float color = (clamp(c, 0.5, 1.0) - 0.5) * 2.0;"

"	gl_FragColor.xyz = (v_color.xyz + v_additive.xyz) * color;"
"	gl_FragColor.w = alpha;"
"	gl_FragColor *= v_color.w;"
"}";

static const char *color_f =
PRECISION
"varying vec2 v_texcoord;"
"varying vec4 v_color;"
"varying vec4 v_additive;"

"uniform sampler2D texture0;"

"void main() {"
"	vec4 tmp = texture2D(texture0, v_texcoord);"
"	gl_FragColor.xyz = v_color.xyz * tmp.w;"
"	gl_FragColor.w = tmp.w;"
"}";

static const char *gray_f =
PRECISION
"varying vec2 v_texcoord;"
"varying vec4 v_color;"
"varying vec4 v_additive;"

"uniform sampler2D texture0;"

"void main() {"
"	vec4 tmp = texture2D(texture0, v_texcoord);"
"	vec4 c;"
"	c.xyz = tmp.xyz * v_color.xyz;"
"	c.w = tmp.w;"
"	c *= v_color.w;"
"	c.xyz += v_additive.xyz * tmp.w;"
"	float g = dot(c.rgb , vec3(0.299, 0.587, 0.114));"
"	gl_FragColor = vec4(g,g,g,c.a);"
"}";

static const char *blend_v =
PRECISION
"attribute vec4 position;"
"attribute vec2 texcoord;"
"attribute vec4 color;"
"attribute vec4 additive;"

"varying vec2 v_texcoord;"
"varying vec2 v_mask_texcoord;"
"varying vec4 v_color;"
"varying vec4 v_additive;"

"uniform vec2 mask;"

"void main() {"
"	gl_Position = position + vec4(-1,1,0,0);"
"	v_texcoord = texcoord;"
"	v_mask_texcoord = texcoord + mask;"
"	v_color = color;"
"	v_additive = additive;"
"}";

static const char *blend_f =
PRECISION
"varying vec2 v_texcoord;"
"varying vec2 v_mask_texcoord;"
"varying vec4 v_color;"
"varying vec4 v_additive;"

"uniform sampler2D texture0;"

"void main() {"
"	vec4 tmp = texture2D(texture0, v_texcoord);"
"	gl_FragColor.xyz = tmp.xyz * v_color.xyz;"
"	gl_FragColor.w = tmp.w;"
"	gl_FragColor *= v_color.w;"
"	gl_FragColor.xyz += v_additive.xyz * tmp.w;"

"	vec4 m = texture2D(texture0, v_mask_texcoord);"
"	gl_FragColor.xyz *= m.xyz;"
//"	gl_FragColor *= m.w;"
"}";

static const char *renderbuffer_v =
PRECISION_HIGH
"attribute vec4 position;"
"attribute vec2 texcoord;"
"attribute vec4 color;"

"varying vec2 v_texcoord;"
"varying vec4 v_color;"

"uniform vec4 st;"

"void main() {"
"	gl_Position.x = position.x * st.x + st.z -1.0;"
"	gl_Position.y = position.y * st.y + st.w +1.0;"
"	gl_Position.z = position.z;"
"	gl_Position.w = position.w;"
"	v_texcoord = texcoord;"
"	v_color = color;"
"}";

static const char *renderbuffer_f =
PRECISION
"varying vec2 v_texcoord;"
"varying vec4 v_color;"
"uniform sampler2D texture0;"

"void main() {"
"	vec4 tmp = texture2D(texture0, v_texcoord);"
"	gl_FragColor.xyz = tmp.xyz * v_color.xyz;"
"	gl_FragColor.w = tmp.w;"
"	gl_FragColor *= v_color.w;"
"}";

void shader_load_glsl(void) {
	shader_load(PROGRAM_PICTURE, sprite_f, sprite_v, 0, 0);
	shader_load(PROGRAM_RENDERBUFFER, renderbuffer_f, renderbuffer_v, 0, 0);
	shader_load(PROGRAM_TEXT, text_f, sprite_v, 0, 0);
	shader_load(PROGRAM_TEXT_EDGE, text_edge_f, sprite_v, 0, 0);
	shader_load(PROGRAM_GRAY, gray_f, sprite_v, 0, 0);
	shader_load(PROGRAM_COLOR, color_f, sprite_v, 0, 0);
	shader_load(PROGRAM_BLEND, blend_f, blend_v, 0, 0);

	shader_add_uniform(PROGRAM_RENDERBUFFER, "st", UNIFORM_FLOAT4);
}