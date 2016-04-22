
local ej = require "pixel"
local geo = require "geometry"
local aoi = require "aoi"

local game = {}

local objs = {}
local lines = {}

local obj_num = 220
local enter_r = 50
local leave_r = 80

math.randomseed(os.time())

for i=1, obj_num do local obj = {}
    obj.id = aoi.enter()
    obj.x = 0
    obj.y = 0
    aoi.locate(obj.id, 0, 0)
    aoi.speed(obj.id, math.random(1, 3))
    aoi.move(obj.id, math.random(1, 1000), math.random(1, 600))
    table.insert(objs, obj)
end


function game.update()
    for i, obj in ipairs(objs) do
        aoi.update(obj.id, 1)
        local t = aoi.trigger(obj.id, enter_r, leave_r)
        for j, k in ipairs(t) do
            if k.what == 1 then
                local o = k.id..' '..obj.id
                lines[o] = {s = obj.id, t = k.id}
            elseif k.what == 2 then
                local o = k.id..' '..obj.id
                lines[o] = nil
            end
        end
        if 1 ~= aoi.moving(obj.id) then
            aoi.speed(obj.id, math.random(1, 3))
            aoi.move(obj.id, math.random(1, 1000), math.random(1, 600))
        else
            obj.x, obj.y = aoi.pos(obj.id)
        end
    end
end

function game.drawframe()
	ej.clear(0xff808080)	-- clear (0.5,0.5,0.5,1) gray
    for i, obj in ipairs(objs) do
        geo.box(obj.x-3, obj.y-3, 6, 6, 0xffffffff)
        geo.box(obj.x-enter_r, obj.y-enter_r, enter_r*2, enter_r*2, 0x6080f0ff)
        geo.box(obj.x-leave_r, obj.y-leave_r, leave_r*2, leave_r*2, 0x2000ffff)
    end
    for i, line in pairs(lines) do
        if line then
            local sx, sy = aoi.pos(line.s)
            local tx, ty = aoi.pos(line.t)
            geo.line(sx, sy, tx, ty, 0xffff0000)
        end
    end
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

