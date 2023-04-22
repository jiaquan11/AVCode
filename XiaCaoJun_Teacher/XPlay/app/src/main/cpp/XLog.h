#ifndef _XLOG_H_
#define _XLOG_H_

#ifdef ANDROID
#include <android/log.h>
#define LOG_TAG "XPlay"
#define  XLOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define  XLOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#else
#define  XLOGI(...)  printf("XPlay", __VA_ARGS__)
#define  XLOGE(...)  printf("XPlay", __VA_ARGS__)
#endif

#endif
