local NIGHT = {
	sky_far			= { 0.00, 0.00, 0.00, 1.00 },
	sky_near		= { 0.12, 0.12, 0.12, 1.00 },

	sea_far			= { 0.02, 0.02, 0.02, 1.00 },
	sea_near		= { 0.08, 0.08, 0.08, 1.00 },
	sea_spec		= { 0.20, 0.20, 0.20, 1.00 },

	moon_scale		= 0.7,
	moon_color		= { 1.00, 1.00, 1.00, 1.00 },
	moon_glow_scale	= 0.8,
	moon_glow_color	= { 1.00, 1.00, 1.00, 1.00 },
	moon_refl_color	= { 0.40, 0.40, 0.40, 1.00 },
}

local DAWN = {
	sky_far			= { 0.00, 0.00, 0.00, 1.00 },
	sky_near		= { 0.12, 0.12, 0.12, 1.00 },

	sea_far			= { 0.02, 0.02, 0.02, 1.00 },
	sea_near		= { 0.08, 0.08, 0.08, 1.00 },
	sea_spec		= { 0.20, 0.20, 0.20, 1.00 },

	sun_scale		= 0.7,
	sun_color		= { 0.00, 0.00, 0.00, 0.00 },
	sun_glow_scale	= 0.5,
	sun_glow_color	= { 1.00, 1.00, 1.00, 1.00 },
	sun_refl_color	= { 0.00, 0.00, 0.00, 1.00 },

	moon_scale		= 0.9,
	moon_color		= { 1.00, 1.00, 1.00, 0.80 },
	moon_glow_scale	= 0.7,
	moon_glow_color	= { 1.00, 1.00, 1.00, 1.00 },
	moon_refl_color	= { 0.00, 0.00, 0.00, 1.00 },
}

local SUNRISE = {
	sky_far			= { 0.21, 0.35, 0.46, 1.00 },
	sky_near		= { 0.70, 0.56, 0.21, 1.00 },

	sea_far			= { 0.50, 0.30, 0.30, 1.00 },
	sea_near		= { 0.20, 0.20, 0.10, 1.00 },
	sea_spec		= { 0.50, 0.50, 0.30, 1.00 },

	sun_scale		= 1.1,
	sun_color		= { 0.92, 0.88, 0.74, 1.00 },
	sun_glow_scale	= 3,
	sun_glow_color	= { 0.50, 0.50, 0.50, 1.00 },
	sun_refl_color	= { 0.00, 0.00, 0.00, 1.00 },

	moon_scale		= 0.9,
	moon_color		= { 1.00, 1.00, 1.00, 0.80 },
	moon_glow_scale	= 0.7,
	moon_glow_color	= { 1.00, 1.00, 1.00, 1.00 },
	moon_refl_color	= { 0.00, 0.00, 0.00, 1.00 },
}

local MORNING = {
	sky_far			= { 0.16, 0.52, 0.80, 1.00 },
	sky_near		= { 1.00, 0.80, 0.30, 1.00 },

	sea_far			= { 0.99, 0.85, 0.58, 1.00 },
	sea_near		= { 0.53, 0.43, 0.32, 1.00 },
	sea_spec		= { 1.00, 1.00, 0.60, 1.00 },

	sun_scale		= 1,
	sun_color		= { 0.92, 0.88, 0.74, 1.00 },
	sun_glow_scale	= 4,
	sun_glow_color	= { 0.50, 0.50, 0.50, 1.00 },
	sun_refl_color	= { 0.80, 0.80, 0.40, 1.00 },
}

local DAY = {
	sky_far			= { 0.17, 0.49, 0.71, 1.00 },
	sky_near		= { 0.60, 0.78, 0.88, 1.00 },

	sea_far			= { 0.70, 0.82, 0.91, 1.00 },
	sea_near		= { 0.06, 0.38, 0.53, 1.00 },
	sea_spec		= { 1.00, 1.00, 1.00, 1.00 },

	sun_scale		= 0.7,
	sun_color		= { 1.00, 1.00, 1.00, 1.00 },
	sun_glow_scale	= 6,
	sun_glow_color	= { 1.00, 1.00, 1.00, 1.00 },
	sun_refl_color	= { 0.80, 0.80, 0.80, 1.00 },
}

local SUNSET = {
	sky_far			= { 0.16, 0.52, 0.80, 1.00 },
	sky_near		= { 1.00, 0.80, 0.30, 1.00 },

	sea_far			= { 0.99, 0.85, 0.58, 1.00 },
	sea_near		= { 0.53, 0.43, 0.32, 1.00 },
	sea_spec		= { 1.00, 1.00, 0.80, 1.00 },

	sun_scale		= 1,
	sun_color		= { 0.92, 0.88, 0.74, 1.00 },
	sun_glow_scale	= 3.5,
	sun_glow_color	= { 0.50, 0.50, 0.50, 1.00 },
	sun_refl_color	= { 1.00, 1.00, 0.80, 1.00 },
}

local EVENING = {
	sky_far			= { 0.08, 0.25, 0.40, 1.00 },
	sky_near		= { 0.80, 0.64, 0.24, 1.00 },

	sea_far			= { 0.89, 0.75, 0.48, 1.00 },
	sea_near		= { 0.43, 0.33, 0.22, 1.00 },
	sea_spec		= { 0.50, 0.50, 0.30, 1.00 },

	sun_scale		= 1.1,
	sun_color		= { 0.92, 0.88, 0.74, 1.00 },
	sun_glow_scale	= 1.5,
	sun_glow_color	= { 0.50, 0.50, 0.50, 1.00 },
	sun_refl_color	= { 0.00, 0.00, 0.00, 1.00 },

	moon_scale		= 1,
	moon_color		= { 1.00, 1.00, 1.00, 0.20 },
	moon_glow_scale	= 0.5,
	moon_glow_color	= { 1.00, 1.00, 1.00, 1.00 },
	moon_refl_color	= { 0.00, 0.00, 0.00, 1.00 },
}

local DUSK = {
	sky_far			= { 0.00, 0.00, 0.00, 1.00 },
	sky_near		= { 0.12, 0.12, 0.12, 1.00 },

	sea_far			= { 0.02, 0.02, 0.02, 1.00 },
	sea_near		= { 0.08, 0.08, 0.08, 1.00 },
	sea_spec		= { 0.20, 0.20, 0.20, 1.00 },

	sun_scale		= 1.1,
	sun_color		= { 0.92, 0.88, 0.74, 1.00 },
	sun_glow_scale	= 0.5,
	sun_glow_color	= { 0.50, 0.50, 0.50, 1.00 },
	sun_refl_color	= { 0.00, 0.00, 0.00, 1.00 },

	moon_scale		= 0.9,
	moon_color		= { 1.00, 1.00, 1.00, 0.80 },
	moon_glow_scale	= 0.7,
	moon_glow_color	= { 1.00, 1.00, 1.00, 1.00 },
	moon_refl_color	= { 0.20, 0.20, 0.20, 1.00 },
}

return {
	NIGHT,		-- 00:00
	NIGHT,		-- 01:00
	NIGHT,		-- 02:00
	NIGHT,		-- 03:00
	NIGHT,		-- 04:00
	DAWN,		-- 05:00
	SUNRISE,	-- 06:00
	MORNING,	-- 07:00
	DAY,		-- 08:00
	DAY,		-- 09:00
	DAY,		-- 10:00
	DAY,		-- 11:00
	DAY,		-- 12:00
	DAY,		-- 13:00
	DAY,		-- 14:00
	DAY,		-- 15:00
	DAY,		-- 16:00
	SUNSET,		-- 17:00
	EVENING,	-- 18:00
	DUSK,		-- 19:00
	NIGHT,		-- 20:00
	NIGHT,		-- 21:00
	NIGHT,		-- 22:00
	NIGHT,		-- 23:00
	NIGHT,		-- 24:00 eq SEQ[1]
}
