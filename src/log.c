#include "pixel.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#ifdef __ANDROID__
#include <android/log.h>
#include <jni.h>
#define plog(f, ...) __android_log_vprint(ANDROID_LOG_DEBUG, "pixel", __VA_ARGS__)
#else
#define plog(f, ...) vfprintf(f, __VA_ARGS__)
#endif

void pixel_log(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	plog(stdout, fmt, ap);
	va_end(ap);
}