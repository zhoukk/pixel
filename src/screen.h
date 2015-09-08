#ifndef _SCREEN_H_
#define _SCREEN_H_

#ifdef __cplusplus
extern "C" {
#endif

#define SCREEN_SCALE 16

	void screen_init(float w, float h, float scale);
	void screen_trans(float *x, float *y);
	void screen_scissor(int x, int y, int w, int h);
	int screen_visible(float x, float y);

#ifdef __cplusplus
};
#endif
#endif // _SCREEN_H_