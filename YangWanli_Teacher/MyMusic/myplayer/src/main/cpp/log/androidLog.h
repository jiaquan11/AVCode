#ifndef MYPLAYER_ANDROIDLOG_H_
#define MYPLAYER_ANDROIDLOG_H_

#include <android/log.h>
#include <jni.h>

#define LOG_DEBUG 1

#define LOG_TAG "MYPLAYER"
#define LOGI(FORMAT, ...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, FORMAT, ##__VA_ARGS__)
#define LOGD(FORMAT, ...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, FORMAT, ##__VA_ARGS__)
#define LOGW(FORMAT, ...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, FORMAT, ##__VA_ARGS__)
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, FORMAT, ##__VA_ARGS__)

#endif //MYPLAYER_ANDROIDLOG_H_
