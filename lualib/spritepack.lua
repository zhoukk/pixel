local texture = require "pixel.texture"
local c = require "pixel.spritepack"

local spritepack = {}
local pattern
local raw
local textures = {}
local packages = {}

local TYPE_PICTURE = assert(c.TYPE_PICTURE)
local TYPE_ANIMATION = assert(c.TYPE_ANIMATION)
local TYPE_POLYGON = assert(c.TYPE_POLYGON)
local TYPE_LABEL = assert(c.TYPE_LABEL)
local TYPE_PANEL = assert(c.TYPE_PANEL)
local TYPE_MATRIX =assert(c.TYPE_MATRIX)

local function pack_picture(src, ret)
	table.insert(ret , c.byte(TYPE_PICTURE))
	local n = #src
	table.insert(ret, c.byte(n))
	local maxid = 0
	for i=1,n do
		local data = src[i]
		local texid = (data.tex or 1) - 1
		if texid > maxid then
			maxid = texid
		end
		table.insert(ret, c.byte(texid))
		for j=1,8 do
			table.insert(ret, c.word(data.src[j]))
		end
		for j=1,8 do
			table.insert(ret, c.int32(data.screen[j]))
		end
	end
	return c.picture_size(n) , maxid
end

local function pack_polygon(src, ret)
	table.insert(ret , c.byte(TYPE_POLYGON))
	local n = #src
	table.insert(ret, c.byte(n))
	local maxid = 0
	local total_point = 0
	for i=1,n do
		local data = src[i]
		local texid = (data.tex or 1) - 1
		if texid > maxid then
			maxid = texid
		end
		table.insert(ret, c.byte(texid))
		local pn = #data.src
		total_point = total_point + pn
		assert(pn == #data.screen)
		table.insert(ret, c.byte(pn/2))
		for j=1,pn do
			table.insert(ret, c.word(data.src[j]))
		end
		for j=1,pn do
			table.insert(ret, c.int32(data.screen[j]))
		end
	end
	return c.polygon_size(n, total_point) , maxid
end

local function is_identity( mat )
	if mat == nil then
		return false
	end
	return mat[1] == 1024 and mat[2] == 0 and mat[3] == 0
			and mat[4] == 1024 and mat[5] == 0 and mat[6] == 0
end

local function pack_part(data, ret)
	if type(data) == "number" then
		table.insert(ret, c.frametag "i")
		table.insert(ret, c.word(data))
		return c.part_size()
	else
		local tag = "i"
		assert(data.index, "frame need an index")
		local mat = data.mat
		if type(mat) == "number" then
			tag = tag .. "M"
		else
			if is_identity(mat) then
				mat = nil
			end
			if mat then
				tag = tag .. "m"
			end
		end
		if data.color and data.color ~= 0xffffffff then
			tag = tag .. "c"
		end
		if data.add and data.add ~= 0 then
			tag = tag .. "a"
		end
		if data.touch then
			tag = tag .. "t"
		end
		table.insert(ret, c.frametag(tag))

		table.insert(ret, c.word(data.index))
		if mat then
			if type(mat) == "number" then
				table.insert(ret, c.int32(mat))
			else
				for i=1,6 do
					table.insert(ret, c.int32(mat[i]))
				end
			end
		end
		if data.color and data.color ~= 0xffffffff then
			table.insert(ret, c.color(data.color))
		end
		if data.add and data.add ~= 0 then
			table.insert(ret, c.color(data.add))
		end
		if data.touch then
			table.insert(ret, c.word(1))
		end
		return c.part_size(mat)
	end
end

local function pack_frame(data, ret)
	local size = 0
	table.insert(ret, c.word(#data))
	for _,v in ipairs(data) do
		local psize = pack_part(v, ret)
		size = size + psize
	end
	return size
end

local function pack_label(data, ret)
	table.insert(ret, c.byte(TYPE_LABEL))
	table.insert(ret, c.byte(data.align))
	table.insert(ret, c.color(data.color))
	table.insert(ret, c.word(data.size))
	table.insert(ret, c.word(data.width))
	table.insert(ret, c.word(data.height))
    table.insert(ret, c.byte(data.edge or 0))
    table.insert(ret, c.byte(data.space_w or 0))
    table.insert(ret, c.byte(data.space_h or 0))
    table.insert(ret, c.byte(data.auto_scale or 0))
	return c.label_size()
end

local function pack_panel(data, ret)
	table.insert(ret, c.byte(TYPE_PANEL))
	table.insert(ret, c.int32(data.width))
	table.insert(ret, c.int32(data.height))
	table.insert(ret, c.byte(data.scissor and 1 or 0))
	return c.panel_size()
end

local function pack_animation(data, ret)
	local size = 0
	local max_id = 0
	table.insert(ret , c.byte(TYPE_ANIMATION))
	local component = assert(data.component)
	table.insert(ret , c.word(#component))
	for _, v in ipairs(component) do
		if v.id and v.id > max_id then
			max_id = v.id
		end
		if v.id == nil then
			assert(v.name, "Anchor need a name")
			v.id = 0xffff	-- Anchor use id 0xffff
		end
		table.insert(ret, c.word(v.id))
		table.insert(ret, c.string(v.name))
		size = size + c.string_size(v.name)
	end
	table.insert(ret, c.word(#data))	-- action number
	local frame = 0
	for _, v in ipairs(data) do
		table.insert(ret, c.string(v.action))
		table.insert(ret, c.word(#v))
		size = size + c.string_size(v.action)
		frame = frame + #v
	end
	table.insert(ret, c.word(frame))	-- total frame
	size = size + c.animation_size(frame, #component, #data)
	for _, v in ipairs(data) do
		for _, f in ipairs(v) do
			local fsz = pack_frame(f, ret)
			size = size + fsz
		end
	end
	return size, max_id
end

function spritepack.pack(data)
	local ret = { texture = 0, maxid = 0, size = 0 , data = {}, export = {} }
	local ani_maxid = 0

	for _,v in ipairs(data) do
		if v.type == "matrix" then
			table.insert(ret.data, c.word(0))	-- dummy id for TYPE_MATRIX
			table.insert(ret.data, c.byte(TYPE_MATRIX))
			local n = #v
			table.insert(ret.data, c.int32(n))
			for _, mat in ipairs(v) do
				for i=1,6 do
					table.insert(ret.data, c.int32(mat[i]))
				end
			end
			ret.size = ret.size + n * 24	-- each matrix 24 bytes
		elseif v.type ~= "particle" then
			local id = assert(tonumber(v.id))
			if id > ret.maxid then
				ret.maxid = id
			end
			local exportname = v.export
			if exportname then
				assert(ret.export[exportname] == nil, "Duplicate export name"..exportname)
				ret.export[exportname] = id
			end
			table.insert(ret.data, c.word(id))
			if v.type == "picture" then
				local sz, texid = pack_picture(v, ret.data)
				ret.size = ret.size + sz
				if texid > ret.texture then
					ret.texture = texid
				end
			elseif v.type == "animation" then
				local sz , maxid = pack_animation(v, ret.data)
				ret.size = ret.size + sz
				if maxid > ani_maxid then
					ani_maxid = maxid
				end
			elseif v.type == "polygon" then
				local sz, texid = pack_polygon(v, ret.data)
				ret.size = ret.size + sz
				if texid > ret.texture then
					ret.texture = texid
				end
			elseif v.type == "label" then
				local sz = pack_label(v, ret.data)
				ret.size = ret.size + sz
			elseif v.type == "panel" then
				local sz = pack_panel(v, ret.data)
				ret.size = ret.size + sz
			else
				error ("Unknown type " .. tostring(v.type))
			end
		end
	end

	if ani_maxid > ret.maxid then
		error ("Invalid id in animation ".. ani_maxid)
	end

	ret.texture = ret.texture + 1
	ret.data = table.concat(ret.data)
	ret.size = ret.size + c.pack_size(ret.maxid, ret.texture)
	return ret
end

local function filepath(filename)
	return string.gsub(pattern, "([^?]*)?([^?]*)", "%1"..filename.."%2")
end

local function load(packname)
	local meta
	local p = {}
	local file = filepath(packname)
	if raw then
		local data = io.open(file..".pi", "rb"):read("*a")
		meta = spritepack.import(data)
	else
		local data = dofile(file..".lua")
		meta = spritepack.pack(data)
	end
	local tex = {}
	for i=1, meta.texture do
		local texfile = file.."."..i..".png"
		tex[i] = #textures
		table.insert(textures, texfile)
		texture.load(tex[i], texfile)
	end
	p.pack = c.new(tex, meta.maxid, meta.size, meta.data, meta.data_size)
	p.export = meta.export
	meta.data = nil
	packages[packname] = p
	return p
end

function spritepack.export(meta)
	local result = { true }
	table.insert(result, c.word(meta.maxid))
	table.insert(result, c.word(meta.texture))
	table.insert(result, c.int32(meta.size))
	table.insert(result, c.int32(#meta.data))
	local s = 0
	for k,v in pairs(meta.export) do
		table.insert(result, c.word(v))
		table.insert(result, c.string(k))
		s = s + 1
	end
	result[1] = c.word(s)
	table.insert(result, meta.data)
	return table.concat(result)
end

function spritepack.import(data)
	local meta = { export = {} }
	local export_n, off = c.import(data, 1, 'w')
	meta.maxid , off = c.import(data, off, 'w')
	meta.texture , off = c.import(data, off, 'w')
	meta.size , off = c.import(data, off, 'i')
	meta.data_size , off = c.import(data, off, 'i')
	for i=1, export_n do
		local id, name
		id, off = c.import(data, off, 'w')
		name, off = c.import(data, off, 's')
		meta.export[name] = id
	end
	meta.data = c.import(data, off, 'p')
	return meta
end

function spritepack.load(t)
	pattern = t.pattern
	raw = t.raw
	for _, v in ipairs(t) do
		load(v)
		collectgarbage "collect"
	end
end

function spritepack.query(packname, name)
	local p = packages[packname]
	if not p then
		p = load(packname)
	end
	local id = name
	if type(name) == "string" then
		id = p.export[name]
	end
	return p.pack, id
end

function spritepack.texture(texfile, reduce)
	local tex = #textures
	table.insert(textures, texfile)
	texture.load(tex, texfile, reduce)
	return tex
end

return spritepack
