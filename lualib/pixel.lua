local fw = require "pixel.framework"
local spritepack = require "spritepack"
local sprite = require "sprite"
local shader = require "shader"

local pixel = {
	TOUCH_BEGIN = 1,
	TOUCH_END = 2,
	TOUCH_MOVE = 3,
	
	KEY_DOWN = 1,
	KEY_UP = 2,
}

pixel.size = assert(fw.size)
pixel.fps = assert(fw.fps)
pixel.font = assert(fw.font)
pixel.read = fw.read
pixel.log = function(...)
	fw.log("[LOG]"..string.format(...))
end
pixel.err = function(...)
	fw.log("[ERR]"..string.format(...))
end

pixel.clear = shader.clear
pixel.shader = shader.new
pixel.label = sprite.label

function pixel.sprite(packname, name)
	local p, id = spritepack.query(packname, name)
	if not p then
		error(string.format("'%s' not found", packname))
	end
	return sprite.new(p, id)
end

function pixel.start(cb)
	fw.PIXEL_FRAME = assert(cb.drawframe)
	fw.PIXEL_ERROR = assert(cb.handle_error)

	fw.PIXEL_TOUCH = cb.touch or function() end
	fw.PIXEL_KEY = cb.key or function() end
	fw.PIXEL_INIT = cb.init or function() end
	fw.PIXEL_UNIT = cb.unit or function() end
	fw.PIXEL_UPDATE = cb.update or function() end
	fw.PIXEL_MESSAGE = cb.message or function() end
	fw.PIXEL_WHEEL = cb.wheel or function() end
	fw.PIXEL_GESTURE = cb.gesture or function() end
	fw.PIXEL_PAUSE = cb.on_pause or function() end
	fw.PIXEL_RESUME = cb.on_resume or function() end
	fw.inject()
end

return pixel
