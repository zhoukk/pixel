local ej = require "pixel"
local pack = require "spritepack"
local particle = require "particle"
ej.font "example/asset/pixel.ttf"
pack.load {	pattern = [[example/asset/?]], "sample"}
particle.preload("example/asset/particle")

local ps = particle.new("fire", function()
	print("particle has gone")
end)
ps.group:ps(160, 240)

local obj = ej.sprite("sample", "mine")
obj.label.text = "Hello World"

local game = {}

function game.update()
	ps:update(0.0333)
  ps.group.frame = ps.group.frame + 1
end

local pos = {x=160, y= 300}
local pos2 = {x=160, y = 240}

function game.drawframe()
  ej.clear()	-- default clear color is black (0,0,0,1)
  ps.group:draw()
  obj:draw(pos)
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


