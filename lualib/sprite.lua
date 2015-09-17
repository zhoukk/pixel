local texture = require "pixel.texture"
local shader = require "shader"
local richtext = require "richtext"
local c = require "pixel.sprite"

local sprite = {}
local sprite_meta = {}
local method = c.method
local get = c.get
local set = c.set

local method_test = method.test
function method:test( ... )
	local s = method_test(self, ...)
	if s then
		return debug.setmetatable(s, sprite_meta)
	end
end

local method_fetch = method.fetch
function method:fetch(val)
	local s = method_fetch(self, val)
	if s then
		return debug.setmetatable(s, sprite_meta)
	end
end

local get_parent = get.parent
function get:parent()
	local p = get_parent(self)
	if p and not getmetatable(p) then
		return debug.setmetatable(p, sprite_meta)
	end
	return p
end

local get_material = get.material
function get:material()
	local m = get_material(self)
	if not m then
		local pid
		m, pid = c.material(self)
		if not m then
			return
		end
		local mate = shader.material(pid)
		setmetatable(m, mate)
	end
	return m
end

local set_text = set.text
function set:text(text)
	if not text or text == "" then
		set_text(self, nil)
	else
		local t = type(text)
		assert(t == "string" or t == "number")
		set_text(self, richtext:format(self, tostring(text)))
	end
end

function sprite_meta.__index(spr, key)
	local m = method[key]
	if m then
		return m
	end
	local g = get[key]
	if g then
		return g(spr)
	end
	local c = method.fetch(spr, key)
	if c then
		return c
	end
	print("unsupport get "..key)
	return nil
end

function sprite_meta.__newindex(spr, key, val)
	local s = set[key]
	if s then
		return s(spr, val)
	end
	method.mount(spr, key, val)
end

function sprite.new(pack, id)
	return debug.setmetatable(c.new(pack, id), sprite_meta)
end

function sprite.panel(tb)
	local panel = c.panel(tb.width, tb.height, tb.scissor)
	if panel then
		panel = debug.setmetatable(panel, sprite_meta)
		if tb.scissor then
			panel.scissor = tb.scissor
		end
		return panel
	end
end

function sprite.label(tb)
	local size = tb.size or tb.height - 2
	local label = c.label(tb.width, tb.height, size, tb.color, tb.align, tb.space_w or 0, tb.space_h or 0, tb.auto_scale or 0, tb.edge or 0)
	if label then
		label = debug.setmetatable(label, sprite_meta)
		if tb.text then
			label.text = tb.text
		end
		return label
	end
end

function sprite.proxy()
	local s = c.proxy()
	return debug.setmetatable(s, sprite_meta)
end

return sprite