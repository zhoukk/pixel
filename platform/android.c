#include <jni.h>
#include <android/log.h>
#include <android/asset_manager_jni.h>

#include "readfile.h"

#include <stdio.h>
#include <stdlib.h>

#include "pixel.h"

static int g_t = 0;

static AAssetManager* assets = 0;
int readable(const char *file) {
	AAsset *asset;
	if (!assets) {
		__android_log_print(ANDROID_LOG_DEBUG, "pixel", "no assets while read:%s", file);
		return 0;
	}
	asset = AAssetManager_open(assets, file, AASSET_MODE_UNKNOWN);
	if (!asset) {
		__android_log_print(ANDROID_LOG_DEBUG, "pixel", "can not read:%s", file);
		return 0;
	}
	AAsset_close(asset);
	__android_log_print(ANDROID_LOG_DEBUG, "pixel", "readable:%s", file);
	return 1;
}

char *readfile(const char *file, int *psize) {
	int size;
	char *data;
	AAsset *asset;
	if (!assets) {
		__android_log_print(ANDROID_LOG_ERROR, "pixel", "no assets while read:%s", file);
		return 0;
	}
	asset = AAssetManager_open(assets, file, AASSET_MODE_UNKNOWN);
	if (!asset) {
		__android_log_print(ANDROID_LOG_ERROR, "pixel", "asset open:%s failed", file);
		return 0;
	}
	size = AAsset_getLength(asset);
	data = malloc(size);
	AAsset_read(asset, data, size);
	AAsset_close(asset);
	if (psize) {
		*psize = size;
	}
	__android_log_print(ANDROID_LOG_DEBUG, "pixel", "asset open:%s size:%d", file, size);
	return data;
}

JNIEXPORT void JNICALL Java_com_zhoukk_pixel_pixellib_init(JNIEnv * env, jobject obj, jobject asset, jint width, jint height) {
	assets = AAssetManager_fromJava(env, asset);
	__android_log_print(ANDROID_LOG_DEBUG, "pixel", "init w:%d h:%d", width, height);
	pixel_start(width, height, "game.lua");
}

JNIEXPORT void JNICALL Java_com_zhoukk_pixel_pixellib_unit(JNIEnv * env, jobject obj) {
	__android_log_print(ANDROID_LOG_DEBUG, "pixel", "unit");
	pixel_close();
}

JNIEXPORT void JNICALL Java_com_zhoukk_pixel_pixellib_update(JNIEnv * env, jobject obj, jfloat t) {
	pixel_update(t);
	g_t = t;
}

JNIEXPORT void JNICALL Java_com_zhoukk_pixel_pixellib_frame(JNIEnv * env, jobject obj) {
	pixel_frame(g_t);
}

JNIEXPORT void JNICALL Java_com_zhoukk_pixel_pixellib_touch(JNIEnv * env, jobject obj, jint id, jint why, jint x, jint y) {
	// __android_log_print(ANDROID_LOG_DEBUG, "pixel", "touch id:%d why:%d x:%d y:%d", id, why, x, y);
	pixel_touch(id, why, x, y);
}

JNIEXPORT void JNICALL Java_com_zhoukk_pixel_pixellib_pause(JNIEnv * env, jobject obj) {
	__android_log_print(ANDROID_LOG_DEBUG, "pixel", "pause");
	pixel_pause();
}

JNIEXPORT void JNICALL Java_com_zhoukk_pixel_pixellib_resume(JNIEnv * env, jobject obj) {
	__android_log_print(ANDROID_LOG_DEBUG, "pixel", "resume");
	pixel_resume();
}
