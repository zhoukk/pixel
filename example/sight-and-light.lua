local pixel = require "pixel"
local geo = require "geometry"
local color = require "pixel.color"

local segments = {
	-- // Border
	{a={x=0,y=0}, b={x=840,y=0}},
	{a={x=840,y=0}, b={x=840,y=360}},
	{a={x=840,y=360}, b={x=0,y=360}},
	{a={x=0,y=360}, b={x=0,y=0}},
	-- // Polygon #1
	{a={x=100,y=150}, b={x=120,y=50}},
	{a={x=120,y=50}, b={x=200,y=80}},
	{a={x=200,y=80}, b={x=140,y=210}},
	{a={x=140,y=210}, b={x=100,y=150}},
	-- // Polygon #2
	{a={x=100,y=200}, b={x=120,y=250}},
	{a={x=120,y=250}, b={x=60,y=300}},
	{a={x=60,y=300}, b={x=100,y=200}},
	-- // Polygon #3
	{a={x=200,y=260}, b={x=220,y=150}},
	{a={x=220,y=150}, b={x=300,y=200}},
	{a={x=300,y=200}, b={x=350,y=320}},
	{a={x=350,y=320}, b={x=200,y=260}},
	-- // Polygon #4
	{a={x=540,y=60}, b={x=560,y=40}},
	{a={x=560,y=40}, b={x=570,y=70}},
	{a={x=570,y=70}, b={x=540,y=60}},
	-- // Polygon #5
	{a={x=650,y=190}, b={x=760,y=170}},
	{a={x=760,y=170}, b={x=740,y=270}},
	{a={x=740,y=270}, b={x=630,y=290}},
	{a={x=630,y=290}, b={x=650,y=190}},
	-- // Polygon #6
	{a={x=600,y=95}, b={x=780,y=50}},
	{a={x=780,y=50}, b={x=680,y=150}},
	{a={x=680,y=150}, b={x=600,y=95}}
}

local Mouse = {
	x= 480,
	y= 320,
}

-- Find intersection of RAY & SEGMENT
function getIntersection(ray,segment)
	-- RAY in parametric: Point + Delta*T1
	local r_px = ray.a.x
	local r_py = ray.a.y
	local r_dx = ray.b.x-ray.a.x
	local r_dy = ray.b.y-ray.a.y
	-- SEGMENT in parametric: Point + Delta*T2
	local s_px = segment.a.x
	local s_py = segment.a.y
	local s_dx = segment.b.x-segment.a.x
	local s_dy = segment.b.y-segment.a.y
	-- Are they parallel? If so, no intersect
	local r_mag = math.sqrt(r_dx*r_dx+r_dy*r_dy)
	local s_mag = math.sqrt(s_dx*s_dx+s_dy*s_dy)
	if r_dx/r_mag==s_dx/s_mag and r_dy/r_mag==s_dy/s_mag then
		-- Unit vectors are the same.
		return nil
	end
	-- // SOLVE FOR T1 & T2
	-- // r_px+r_dx*T1 = s_px+s_dx*T2 && r_py+r_dy*T1 = s_py+s_dy*T2
	-- // ==> T1 = (s_px+s_dx*T2-r_px)/r_dx = (s_py+s_dy*T2-r_py)/r_dy
	-- // ==> s_px*r_dy + s_dx*T2*r_dy - r_px*r_dy = s_py*r_dx + s_dy*T2*r_dx - r_py*r_dx
	-- // ==> T2 = (r_dx*(s_py-r_py) + r_dy*(r_px-s_px))/(s_dx*r_dy - s_dy*r_dx)
	local T2 = (r_dx*(s_py-r_py) + r_dy*(r_px-s_px))/(s_dx*r_dy - s_dy*r_dx)
	local T1 = (s_px+s_dx*T2-r_px)/r_dx
	-- // Must be within parametic whatevers for RAY/SEGMENT
	if T1<0 then return nil end
	if T2<0 or T2>1 then return nil end
	-- // Return the POINT OF INTERSECTION
	return {
		x= r_px+r_dx*T1,
		y= r_py+r_dy*T1,
		param= T1
	}
end

function getSightPolygon(sightX,sightY)
	-- // Get all unique points
    local points = {}
    for i, v in ipairs(segments) do
        table.insert(points, v.a)
        table.insert(points, v.b)
    end
	local uniquePoints = {}
    local keys = {}
    for i, v in ipairs(points) do
        local key = v.x..","..v.y
        if not keys[key] then
            table.insert(uniquePoints, v)
            keys[key] = true
        end
    end
	-- // Get all angles
	local uniqueAngles = {}
    for i, v in ipairs(uniquePoints) do
        local uniquePoint = v
		local angle = math.atan2(uniquePoint.y-sightY,uniquePoint.x-sightX)
		uniquePoint.angle = angle
		table.insert(uniqueAngles, angle-0.00001)
        table.insert(uniqueAngles, angle)
        table.insert(uniqueAngles, angle+0.00001)
    end
	-- // RAYS IN ALL DIRECTIONS
	local intersects = {}
    for i, v in ipairs(uniqueAngles) do
        local angle = v
		-- // Calculate dx & dy from angle
		local dx = math.cos(angle)
		local dy = math.sin(angle)
		-- // Ray from center of screen to mouse
		local ray = {
			a={x=sightX,y=sightY},
			b={x=sightX+dx,y=sightY+dy},
		}
		-- // Find CLOSEST intersection
		local closestIntersect = nil
        for i, v in ipairs(segments) do
            local intersect = getIntersection(ray,v)
			if intersect then
    			if not closestIntersect or intersect.param<closestIntersect.param then
    				closestIntersect=intersect
    			end
            end
        end
		-- // Intersect angle
		if closestIntersect then
            closestIntersect.angle = angle
		    -- // Add to list of intersects
            table.insert(intersects,closestIntersect)
        end
    end
	-- // Sort intersects by angle
	table.sort(intersects, function(a,b)
		return a.angle<b.angle
	end)
	-- // Polygon is intersects, in order of angle
	return intersects
end

function drawPolygon(polygon,c)
    local n = #polygon
    local triangle = {}
    for i=1, n do
        local a = polygon[i]
        if i == n then
            i = 0
        end
        local b = polygon[i+1]
        triangle[1] = a.x;
        triangle[2] = a.y;
        triangle[3] = b.x;
        triangle[4] = b.y;
        triangle[5] = Mouse.x;
        triangle[6] = Mouse.y;
        geo.polygon(triangle, color.mul(0x80ffffff, c))
    end
	-- for i, v in ipairs(polygon) do
	-- 	geo.line(Mouse.x, Mouse.y, v.x, v.y, 0xffff0000)
	-- end
end

pixel.start {
    update = function()

    end,
    drawframe = function()
        pixel.clear()

        for i, v in ipairs(segments) do
            geo.line(v.a.x, v.a.y, v.b.x, v.b.y, 0xffffffff)
        end
        -- // Sight Polygons
        local fuzzyRadius = 10
        local polygons = {getSightPolygon(Mouse.x,Mouse.y)}
        -- for i=1, 10 do
        --     local angle = (i-1)*math.pi*2/10
        --     local dx = math.cos(angle)*fuzzyRadius
        --     local dy = math.sin(angle)*fuzzyRadius
        --     table.insert(polygons, getSightPolygon(Mouse.x+dx,Mouse.y+dy))
        -- end
        -- // DRAW AS A GIANT POLYGON
        for i, v in ipairs(polygons) do
            if i > 1 then
                drawPolygon(v,0x20808080)
            end
        end
        drawPolygon(polygons[1],0xff808080)
    end,
    touch = function(what, x, y)
        Mouse.x = x
        Mouse.y = y
    end,
    handle_error = function(...)
        pixel.err(...)
    end
}
