-- This example show how user defined shader works

local ej = require "pixel"
local pack = require "spritepack"
local shader = require "shader"

pack.load {
	pattern = [[example/asset/?]],
	"sample",
}


-- define a shader
local s = ej.shader {
	fs = [[
varying vec2 v_texcoord;
uniform vec4 color;
uniform sampler2D texture0;

void main() {
	vec4 tmp = texture2D(texture0, v_texcoord);
	gl_FragColor = color + tmp;
}
	]],
	uniform = {
		{
			name = "color",
			type = "float4",
		}
	},
	texture = {
		"texture0",
	}
}

s.color(1,0,0,1)	-- set shader color


local obj = ej.sprite("sample","cannon")
obj:ps(100,100)
obj.program = s.id
obj.turret.program = s.id
obj.turret.material:color(0,0,1,1)


local obj2 = ej.sprite("sample","cannon")
obj2:ps(200,100)

obj2.turret.program = s.id
obj2.turret.material:color(0,1,0,1)	-- uniform can be set in material


local obj3 = ej.sprite("sample","cannon")
obj3:ps(300,100)
obj3.program = s.id

local game = {}

function game.update()
end

function game.drawframe()
	ej.clear(0xff808080)	-- clear (0.5,0.5,0.5,1) gray
	obj:draw()
	obj2:draw()
	obj3:draw()
end

function game.touch(what, x, y)
end

function game.message(...)
end

function game.handle_error(...)
end

function game.on_resume()
end

function game.on_pause()
end

ej.start(game)

