local ej = require "pixel"
local pack = require "spritepack"
ej.font "example/asset/pixel.ttf"

package.path = package.path..";example/dawn/?.lua"

local shader = require "dawn_shader"
local scene = require "dawn_scene"

-- init
local function init()
	math.randomseed(os.time())

	pack.load {
		pattern = "example/asset/dawn/?",
		"dawn",
	}

	shader:init()
	scene:init()
end

init()
-- game callback
local game = {}

function game.update()
	scene:update()
end

function game.drawframe()
	ej.clear(0xff000000)
	scene:draw()
end

local last_x
function game.touch(what, x, y)
	if what == ej.TOUCH_BEGIN then
		scene:pause_time(true)
	elseif what == ej.TOUCH_END then
		scene:pause_time(false)
	elseif what == ej.TOUCH_MOVE then
		scene:shift_time(x - last_x)
	end

	last_x = x
end

function game.message(id, state, data, n)
	if data == "LEFT" or data == "RIGHT" then
		scene:layout_sim(true)
	else
		scene:layout_sim(false)
	end
end

function game.handle_error(...)
end

function game.on_resume()
end

function game.on_pause()
end

ej.start(game)
