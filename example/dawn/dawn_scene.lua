local ej = require "pixel"
local shader = require "dawn_shader"
local CONFIG = require "dawn_config"

local unpack = table.unpack
local format = string.format

local sw = 1024
local sh = 768

local SKY_PCT = 0.6
local STAR_PCT = SKY_PCT * 0.9
local OBJ_ORB = 0.45
local OBJ_SCALE = 768 / 768

local SIM_SPEED = 0.02
local WAVE_SPEED = 0.05
local SHIFT_SPEED = 0.01

local SKY_TEX_W = 1
local SKY_TEX_H = 1
local SEA_TEX_W = 64
local SEA_TEX_H = 64

local SL8 = math.pow(2, 8)
local SL16 = math.pow(2, 16)
local SL24 = math.pow(2, 24)
local PIOVER12 = math.pi / 12

local M = {}

local function _mix1(c1, c2, f)
	return c1*(1-f) + c2*f
end

local function _mix4(c1, c2, f)
	return {
		c1[1]*(1-f) + c2[1]*f,
		c1[2]*(1-f) + c2[2]*f,
		c1[3]*(1-f) + c2[3]*f,
		c1[4]*(1-f) + c2[4]*f,
	}
end

local function _mixc(c1, c2, f)
	local c = _mix4(c1, c2, f)
	return
		math.floor(c[4]*255) * SL24 +
		math.floor(c[1]*255) * SL16 +
		math.floor(c[2]*255) * SL8 +
		math.floor(c[3]*255)
end

function M:init()
	self:init_sim()
	self:layout_sim(false)
end

function M:update()
end

function M:draw()
	self:update_sim()
end

function M:pause_time(p)
	self.v_pause = p
end

function M:shift_time(d)
	self.v_time = self.v_time - d * SHIFT_SPEED
end

function M:init_sim()
	-- sky
	self.v_sky = ej.sprite("dawn", "blank")
	self.v_sky.program = shader.v_sky.id

	-- sea
	self.v_sea = ej.sprite("dawn", "noise")
	self.v_sea.program = shader.v_sea.id

	-- sun
	self.v_sun = ej.sprite("dawn", "sun")
	self.v_sun_glow = ej.sprite("dawn", "glow")
	self.v_sun_glow.program = shader.v_glow.id

	-- moon
	self.v_moon = ej.sprite("dawn", "moon")
	self.v_moon_glow = ej.sprite("dawn", "glow")
	self.v_moon_glow.program = shader.v_glow.id

	-- star
	self.v_stars = {}
	self.v_star_a = {}
	for i=1,100 do
		local star = ej.sprite("dawn", "star")
		table.insert(self.v_stars, star)
	end

	-- hud
	self.v_label = ej.sprite("dawn", "default_label")

	-- init day
	self.v_time = 8
	self.v_pause = false

	self.v_t0x = 0  -- shader params
	self.v_t01 = 0
	self.v_t01_dir = 1
end

function M:layout_sim(horz)
	if horz then
		sw = 768
		sh = 1024
	else
		sw = 1024
		sh = 768
	end

	self.v_sky:ps(sw*0.5, sh*SKY_PCT*0.5 + 1)
	self.v_sky:sr(sw/SKY_TEX_W, sh*SKY_PCT/SKY_TEX_H + 2)

	self.v_sea:ps(sw*0.5, sh*(1+SKY_PCT)*0.5)
	self.v_sea:sr(sw/SEA_TEX_W, sh*(1-SKY_PCT)/SEA_TEX_H)

	local star_h = math.floor(sh * STAR_PCT)
	for i=1,100 do
		local x = math.random(0, sw)
		local y = math.random(0, star_h)
		local s = math.random()*0.6 + 0.1
		local a = 1 - y / star_h
		self.v_stars[i]:ps(x, y, s)
		self.v_star_a[i] = a
	end

	self.v_label:ps(10, 10)
end

function M:update_sim()
	-- time
	if not self.v_pause then
		self.v_time = self.v_time + SIM_SPEED
	end
	while self.v_time < 0 do
		self.v_time = self.v_time + 24
	end
	while self.v_time > 24 do
		self.v_time = self.v_time - 24
	end

	local h = math.floor(self.v_time)
	local m = math.floor(self.v_time - h)

	self.v_label.text = format("%02d:%02d", h, m*60)

	local s1 = CONFIG[h+1]
	local s2 = CONFIG[h+2]

	-- sky
	shader.v_sky.far(unpack(_mix4(s1.sky_far, s2.sky_far, m)))
	shader.v_sky.near(unpack(_mix4(s1.sky_near, s2.sky_near, m)))
	self.v_sky:draw()

	-- star
	if self.v_time > 18 or self.v_time < 6 then
		local fa = 1
		if self.v_time > 18 then
			fa = math.sin((self.v_time - 18) * PIOVER12)
		else
			fa = math.sin((self.v_time + 6) * PIOVER12)
		end

		for i=1,100 do
			local a = self.v_star_a[i]
			a = a * (math.random()*0.4 + 0.6) * fa
			self.v_stars[i].color = 0xffffff + math.floor(a*255) * SL24
			self.v_stars[i]:draw()
		end
	end

	-- sun & moon
	local rx = sw * OBJ_ORB
	local ry = sh * OBJ_ORB
	local x, y, s, c, gs, gc, rc

	if self.v_time > 5 and self.v_time < 19 then
		x = sw*0.5 + math.cos((self.v_time-6)*PIOVER12)*rx
		y = sh*SKY_PCT*1.1 - math.sin((self.v_time-6)*PIOVER12)*ry
		s = _mix1(s1.sun_scale, s2.sun_scale, m)
		c = _mixc(s1.sun_color, s2.sun_color, m)
		gs = _mix1(s1.sun_glow_scale, s2.sun_glow_scale, m)
		gc = _mixc(s1.sun_glow_color, s2.sun_glow_color, m)
		rc = _mix4(s1.sun_refl_color, s2.sun_refl_color, m)

		self.v_sun_glow:ps(x, y, gs*OBJ_SCALE)
		self.v_sun_glow.color = gc
		self.v_sun_glow:draw()
		self.v_sun:ps(x, y, s*OBJ_SCALE)
		self.v_sun.color = c
		self.v_sun:draw()
	end

	if self.v_time > 18 or self.v_time < 6 then
		x = sw*0.5 + math.cos((self.v_time-18)*PIOVER12)*rx
		y = sh*SKY_PCT*1.1 - math.sin((self.v_time-18)*PIOVER12)*ry
		s = _mix1(s1.moon_scale, s2.moon_scale, m)
		c = _mixc(s1.moon_color, s2.moon_color, m)
		gs = _mix1(s1.moon_glow_scale, s2.moon_glow_scale, m)
		gc = _mixc(s1.moon_glow_color, s2.moon_glow_color, m)
		rc = _mix4(s1.moon_refl_color, s2.moon_refl_color, m)

		self.v_moon_glow:ps(x, y, gs*OBJ_SCALE)
		self.v_moon_glow.color = gc
		self.v_moon_glow:draw()
		self.v_moon:ps(x, y, s*OBJ_SCALE)
		self.v_moon.color = c
		self.v_moon:draw()
	end

	-- sea
	self.v_t0x = self.v_t0x + WAVE_SPEED
	self.v_t01 = self.v_t01 + WAVE_SPEED*self.v_t01_dir
	if self.v_t01 > 1 or self.v_t01 < 0 then
		self.v_t01_dir = -self.v_t01_dir
	end

	shader.v_sea.t(self.v_t0x)
	shader.v_sea.t1(self.v_t01)
	shader.v_sea.sx(x / sw)
	shader.v_sea.far(unpack(_mix4(s1.sea_far, s2.sea_far, m)))
	shader.v_sea.near(unpack(_mix4(s1.sea_near, s2.sea_near, m)))
	shader.v_sea.spec(unpack(_mix4(s1.sea_spec, s2.sea_spec, m)))
	shader.v_sea.refl(unpack(rc))
	self.v_sea:draw()

	-- hud
	self.v_label:draw()
end

return M
