local ej = require "pixel"
local pack = require "spritepack"
local sprite = require "sprite"
local renderbuffer = require "pixel.renderbuffer"

pack.load {
	pattern = [[example/asset/?]],
	"birds",
}

local bird = ej.sprite("birds","bird")
local rb = renderbuffer.new()

for i=1, 100 do
	rb:add(bird)
	bird:ps(5 *i, 5 *i)
end
rb:update()


local game = {}
local screencoord = { x = 512, y = 384, scale = 1.2 }

local p = 0

function game.update()
	p=p+1
end

bird:ps(200,0)

function game.drawframe()
	ej.clear()
	rb:draw(p,p)
	bird:draw(screencoord)
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


