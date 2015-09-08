local c = require "pixel.shader"

local shader = {}

BLEND_ZERO = 0
BLEND_ONE = 1
BLEND_SRC_COLOR = 0x300
BLEND_ONE_MINUS_SRC_COLOR = 0x301
BLEND_SRC_ALPHA = 0x302
BLEND_ONE_MINUS_SRC_ALPHA = 0x303
BLEND_DST_ALPHA = 0x304
BLEND_ONE_MINUS_DST_ALPHA = 0x0305
BLEND_DST_COLOR = 0x0306
BLEND_ONE_MINUS_DST_COLOR = 0x0307
BLEND_SRC_ALPHA_SATURATE = 0x0308

local PRECISION = ""
local PRECISION_HIGH = ""

if c.version() == 2 then
	-- Opengl ES 2.0 need float precision specifiers
	PRECISION = "precision lowp float;\n"
	PRECISION_HIGH = "precision highp float;\n"
end

local sprite_fs = [[
varying vec2 v_texcoord;
varying vec4 v_color;
varying vec4 v_additive;
uniform sampler2D texture0;

void main() {
	vec4 tmp = texture2D(texture0, v_texcoord);
	gl_FragColor.xyz = tmp.xyz * v_color.xyz;
	gl_FragColor.w = tmp.w;
	gl_FragColor *= v_color.w;
	gl_FragColor.xyz += v_additive.xyz * tmp.w;
}
]]

local sprite_vs = [[
attribute vec4 position;
attribute vec2 texcoord;
attribute vec4 color;
attribute vec4 additive;

varying vec2 v_texcoord;
varying vec4 v_color;
varying vec4 v_additive;

void main() {
	gl_Position = position + vec4(-1.0,1.0,0,0);
	v_texcoord = texcoord;
	v_color = color;
	v_additive = additive;
}
]]

local text_fs = [[
varying vec2 v_texcoord;
varying vec4 v_color;
varying vec4 v_additive;

uniform sampler2D texture0;

void main() {
	float c = texture2D(texture0, v_texcoord).w;
	float alpha = clamp(c, 0.0, 0.5) * 2.0;

	gl_FragColor.xyz = (v_color.xyz + v_additive.xyz) * alpha;
	gl_FragColor.w = alpha;
	gl_FragColor *= v_color.w;
}
]]

local text_edge_fs = [[
varying vec2 v_texcoord;
varying vec4 v_color;
varying vec4 v_additive;

uniform sampler2D texture0;

void main() {
	float c = texture2D(texture0, v_texcoord).w;
	float alpha = clamp(c, 0.0, 0.5) * 2.0;
	float color = (clamp(c, 0.5, 1.0) - 0.5) * 2.0;

	gl_FragColor.xyz = (v_color.xyz + v_additive.xyz) * color;
	gl_FragColor.w = alpha;
	gl_FragColor *= v_color.w;
}
]]

local gray_fs = [[
varying vec2 v_texcoord;
varying vec4 v_color;
varying vec4 v_additive;

uniform sampler2D texture0;

void main()
{
	vec4 tmp = texture2D(texture0, v_texcoord);
	vec4 c;
	c.xyz = tmp.xyz * v_color.xyz;
	c.w = tmp.w;
	c *= v_color.w;
	c.xyz += v_additive.xyz * tmp.w;
	float g = dot(c.rgb , vec3(0.299, 0.587, 0.114));
	gl_FragColor = vec4(g,g,g,c.a);
}
]]

local color_fs = [[
varying vec2 v_texcoord;
varying vec4 v_color;
varying vec4 v_additive;

uniform sampler2D texture0;

void main()
{
	vec4 tmp = texture2D(texture0, v_texcoord);
	gl_FragColor.xyz = v_color.xyz * tmp.w;
	gl_FragColor.w = tmp.w;
}
]]


local blend_fs = [[
varying vec2 v_texcoord;
varying vec2 v_mask_texcoord;
varying vec4 v_color;
varying vec4 v_additive;

uniform sampler2D texture0;

void main() {
	vec4 tmp = texture2D(texture0, v_texcoord);
	gl_FragColor.xyz = tmp.xyz * v_color.xyz;
	gl_FragColor.w = tmp.w;
	gl_FragColor *= v_color.w;
	gl_FragColor.xyz += v_additive.xyz * tmp.w;

	vec4 m = texture2D(texture0, v_mask_texcoord);
	gl_FragColor.xyz *= m.xyz;
//	gl_FragColor *= m.w;
}
]]


local blend_vs = [[
attribute vec4 position;
attribute vec2 texcoord;
attribute vec4 color;
attribute vec4 additive;

varying vec2 v_texcoord;
varying vec2 v_mask_texcoord;
varying vec4 v_color;
varying vec4 v_additive;

uniform vec2 mask;

void main() {
	gl_Position = position + vec4(-1,1,0,0);
	v_texcoord = texcoord;
	v_mask_texcoord = texcoord + mask;
	v_color = color;
    v_additive = additive;
}
]]

local renderbuffer_fs = [[
varying vec2 v_texcoord;
varying vec4 v_color;
uniform sampler2D texture0;

void main() {
	vec4 tmp = texture2D(texture0, v_texcoord);
	gl_FragColor.xyz = tmp.xyz * v_color.xyz;
	gl_FragColor.w = tmp.w;
	gl_FragColor *= v_color.w;
}
]]

local renderbuffer_vs = [[
attribute vec4 position;
attribute vec2 texcoord;
attribute vec4 color;

varying vec2 v_texcoord;
varying vec4 v_color;

uniform vec4 st;

void main() {
	gl_Position.x = position.x * st.x + st.z -1.0;
	gl_Position.y = position.y * st.y + st.w +1.0;
	gl_Position.z = position.z;
	gl_Position.w = position.w;
	v_texcoord = texcoord;
	v_color = color;
}
]]

PROGRAM_DEFAULT = -1
PROGRAM_PICTURE = 0
PROGRAM_RENDERBUFFER = 1
PROGRAM_TEXT = 2
PROGRAM_TEXT_EDGE = 3
PROGRAM_GRAY = 4
PROGRAM_COLOR = 5
PROGRAM_BLEND = 6

c.load(PROGRAM_PICTURE, PRECISION .. sprite_fs, PRECISION .. sprite_vs)
c.load(PROGRAM_TEXT, PRECISION .. text_fs, PRECISION .. sprite_vs)
c.load(PROGRAM_TEXT_EDGE, PRECISION .. text_edge_fs, PRECISION .. sprite_vs)
c.load(PROGRAM_GRAY, PRECISION .. gray_fs, PRECISION .. sprite_vs)
c.load(PROGRAM_COLOR, PRECISION .. color_fs, PRECISION .. sprite_vs)
c.load(PROGRAM_BLEND, PRECISION .. blend_fs, PRECISION .. blend_vs)
c.load(PROGRAM_RENDERBUFFER, PRECISION .. renderbuffer_fs, PRECISION_HIGH .. renderbuffer_vs)
c.uniform_bind(PROGRAM_RENDERBUFFER, {{name="st", type=4}})


local PROGRAM_USER = 7
local uniform_format = {
	float = 1,
	float2 = 2,
	float3 = 3,
	float4 = 4,
	matrix33 = 5,
	matrix44 = 6,
}

local shader_material = {}
local function material_meta(id, arg)
	local uniform = arg.uniform
	local meta
	if uniform then
		local index_meta = {}
		meta = {__index = index_meta}
		for i, u in ipairs(uniform) do
			local loc = i-1
			index_meta[u.name] = function(self, ...)
				c.material_uniform(self.__obj, loc, ...)
			end
		end
		if arg.texture then
			index_meta.texture = function(self, ...)
				c.material_texture(self.__obj, ...)
			end
		end
	end
	shader_material[id] = meta
end

function shader.new(arg)
	local id = PROGRAM_USER
	PROGRAM_USER = id + 1
	local vs = PRECISION .. (arg.vs or sprite_vs)
	local fs = PRECISION_HIGH .. (arg.fs or sprite_fs)
	c.load(id, fs, vs, arg.texture)
	local uniform = arg.uniform
	if uniform then
		for _, v in ipairs(uniform) do
			v.type = assert(uniform_format[v.type])
		end
		c.uniform_bind(id, uniform)
	end

	local s = {}
	s.id = id
	if uniform then
		for i, u in ipairs(uniform) do
			local loc = i-1
			local format = u.type
			s[u.name] = function(...)
				c.uniform_set(id, loc, format, ...)
			end
		end
	end
	material_meta(id, arg)
	return s
end

function shader.material(pid)
	return shader_material[pid]
end

shader.clear = assert(c.clear)
shader.draw = assert(c.draw)
shader.blend = assert(c.blend)

return shader