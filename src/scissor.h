#ifndef _SCISSOR_H_
#define _SCISSOR_H_

#ifdef __cplusplus
extern "C" {
#endif

	void scissor_push(int x, int y, int w, int h);
	void scissor_pop(void);

#ifdef __cplusplus
};
#endif
#endif // _SCISSOR_H_