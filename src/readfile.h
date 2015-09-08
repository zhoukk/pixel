#ifndef _READFILE_H_
#define _READFILE_H_

#ifdef __cplusplus
extern "C" {
#endif

	int readable(const char *file);
	char *readfile(const char *file, int *psize);

#ifdef __cplusplus
};
#endif
#endif // _READFILE_H_