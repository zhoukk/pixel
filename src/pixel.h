#ifndef _PIXEL_H_
#define _PIXEL_H_

#ifdef __cplusplus
extern "C" {
#endif

#define TOUCH_BEGIN  0x001
#define TOUCH_END    0x002
#define TOUCH_MOVE   0x003

#define KEY_DOWN	0x01
#define KEY_UP		0x02

	void pixel_start(int w, int h, const char *start);
	void pixel_close(void);
	void pixel_update(float t);
	void pixel_frame(float t);
	void pixel_touch(int id, int what, int x, int y);
	void pixel_wheel(int delta, int x, int y);
	void pixel_key(int what, char ch);
	void pixel_gesture(int type, int x1, int y1, int x2, int y2, int s);
	void pixel_pause(void);
	void pixel_resume(void);
	void pixel_message(const char *msg);

	void pixel_log(const char *fmt, ...);
	int pixel_fps(int fps);

#ifdef PIXEL_LUA
#include "lua.h"
	int pixel_init(lua_State *L);
	int pixel_unit(lua_State *L);
	int pixel_flush(lua_State *L);
	int pixel_viewport(lua_State *L);
#endif // PIXEL_LUA

#ifdef __cplusplus
};
#endif
#endif // _PIXEL_H_