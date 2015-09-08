#ifndef _SPRITEPACK_H_
#define _SPRITEPACK_H_

#include <stdint.h>

#define TYPE_EMPTY 0
#define TYPE_PICTURE 1
#define TYPE_ANIMATION 2
#define TYPE_POLYGON 3
#define TYPE_LABEL 4
#define TYPE_PANEL 5
#define TYPE_ANCHOR 6
#define TYPE_MATRIX 7

#define TAG_ID 0x0001
#define TAG_COLOR 0x0002
#define TAG_ADDITIVE 0x0004
#define TAG_MATRIX 0x0008
#define TAG_TOUCH 0x0010
#define TAG_MATRIXREF 0x0020

#define ANCHOR_ID 0xffff

#ifdef __cplusplus
extern "C" {
#endif

	struct matrix;

	struct pack_panel {
		uint16_t width;
		uint16_t height;
		uint8_t scissor;
	};

	struct pack_label {
		uint32_t color;
		uint16_t width;
		uint16_t height;
		uint16_t size;
		uint8_t align;
		uint8_t edge;
		uint16_t space_h;
		uint16_t space_w;
		uint8_t auto_scale;
	};

	struct pack_quad {
		uint16_t texid;
		uint16_t texture_coord[8];
		int32_t screen_coord[8];
	};

	struct pack_picture {
		uint16_t n;
		struct pack_quad rect[1];
	};

	struct pack_poly {
		uint16_t n;
		uint16_t texid;
		uint16_t *texture_coord;
		int32_t *screen_coord;
	};

	struct pack_polygon {
		uint16_t n;
		struct pack_poly poly[1];
	};

	struct sprite_trans {
		struct matrix *mat;
		uint32_t color;
		uint32_t addi;
		int16_t pid;
	};

	struct pack_part {
		struct sprite_trans t;
		uint16_t component_id;
		uint8_t touchable;
	};

	struct pack_frame {
		struct pack_part *part;
		uint16_t n;
	};

	struct pack_action {
		const uint8_t *name;
		uint16_t n;
		uint16_t start_frame;
	};

	struct pack_component {
		const uint8_t *name;
		uint16_t id;
	};

	struct pack_animation {
		struct pack_frame *frame;
		struct pack_action *action;
		uint16_t frame_n;
		uint16_t action_n;
		uint16_t component_n;
		struct pack_component component[1];
	};

	struct sprite_pack {
		int *type;
		void **data;
		int n;
	};

	void spritepack_init(const char *path);
	void spritepack_unit(void);
	struct sprite_pack *spritepack_load(const char *file);
	struct sprite_pack *spritepack_query(const char *file);
	int spritepack_id(const char *file, const char *name);

#ifdef PIXEL_LUA
#include "lua.h"
	int pixel_spritepack(lua_State *L);
#endif // PIXEL_LUA

#ifdef __cplusplus
};
#endif
#endif // _SPRITEPACK_H_