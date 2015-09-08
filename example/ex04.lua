local ej = require "pixel"
local pack = require "spritepack"
local sprite = require "sprite"
ej.font "example/asset/pixel.ttf"
pack.load {
	pattern = [[example/asset/?]],
	"sample",
}

local scissor = false
local obj = ej.sprite("sample","mine")
obj.resource.frame = 70
obj.label.text = "The #[red]quick#[green] brown #[blue]fox jumps#[stop] over the lazy dog"
obj:ps(400,300)
local screencoord = { scale = 1.2 }

local game = {}

function game.update()
	obj.frame = obj.frame + 1
end

function game.drawframe()
	ej.clear()
	obj:draw(screencoord)
end

function game.touch(what, x, y)
	if what == ej.TOUCH_END then
		local touched = obj:test(x,y,screencoord)
		if touched then
			if touched.name == "label" then
				touched.text = "label touched"
			end
			if touched.name == "panel" then
				scissor = not scissor
				touched.scissor = scissor
				obj.label.text = scissor and "Set scissor" or "Clear scissor"
			end
		else
			obj.label.text = "Not Hit"
		end
	end
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


