local c = require "pixel.geometry"
local shader = require "shader"

local geometry = {}

local s = shader.new {
	vs = [[
attribute vec4 position;
attribute vec4 color;

varying vec4 v_color;

void main() {
	gl_Position = position + vec4(-1.0,1.0,0,0);
	v_color = color;
}
	]],
	fs = [[
varying vec4 v_color;

void main() {
	gl_FragColor = v_color;
}
	]]
}

c.program(s.id)

geometry.line = assert(c.line)
geometry.box = assert(c.box)
geometry.polygon = assert(c.polygon)

return geometry