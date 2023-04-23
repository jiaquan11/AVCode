#include <jni.h>
#include <string>
#include <android/native_window_jni.h>
#include "IPlayerProxy.h"

//系统自动加载调用，配置硬解环境
extern "C" JNIEXPORT
jint JNI_OnLoad(JavaVM *vm, void *res) {
    IPlayerProxy::Get()->Init(vm);
    IPlayerProxy::Get()->isHardDecode = true;//默认硬解
    return JNI_VERSION_1_4;
}

extern "C" JNIEXPORT void JNICALL
Java_com_jiaquan_xplay_XPlay_InitView(JNIEnv *env, jobject thiz, jobject surface) {
    ANativeWindow *win = ANativeWindow_fromSurface(env, surface);
    IPlayerProxy::Get()->InitView(win);
}

extern "C" JNIEXPORT void JNICALL
Java_com_jiaquan_xplay_XPlay_PlayOrPause(JNIEnv *env, jobject thiz) {
    IPlayerProxy::Get()->SetPause(!IPlayerProxy::Get()->IsPause());
}

extern "C" JNIEXPORT void JNICALL
Java_com_jiaquan_xplay_OpenUrl_Open(JNIEnv *env, jobject thiz, jstring urlStr) {
    const char *url = env->GetStringUTFChars(urlStr, 0);
    XLOGI("XPlay Open");
    IPlayerProxy::Get()->Open(url);
    IPlayerProxy::Get()->Start();
    env->ReleaseStringUTFChars(urlStr, url);
}

extern "C" JNIEXPORT jdouble JNICALL
Java_com_jiaquan_xplay_MainActivity_PlayPos(JNIEnv *env, jobject thiz) {
    return IPlayerProxy::Get()->PlayPos();
}

extern "C" JNIEXPORT void JNICALL
Java_com_jiaquan_xplay_MainActivity_Seek(JNIEnv *env, jobject thiz, jdouble pos) {
    IPlayerProxy::Get()->Seek(pos);
}

