#include <pthread.h>
#include <jni.h>
#include <string>

#include "log/android_log.h"
#include "call_java.h"
#include "wl_ffmpeg.h"


extern "C" {
#include "include/libavformat/avformat.h"
#include "include/libavcodec/avcodec.h"
#include "include/libavutil/avutil.h"
}

#define DELETE_LOCAL_REF(env, obj)  if(obj!=NULL){env->DeleteLocalRef(obj);obj=NULL;}
static const char *const kClassPathName = "com/jiaquan/myplayer/player/WLPlayer";

//全局变量
JavaVM *g_JavaVm = NULL;
CallJava *g_CallJava = NULL;
WLFFmpeg *g_WlFFmpeg = NULL;
WLPlayStatus *g_PlayStatus = NULL;
bool g_IsExit = false;

JNIEXPORT void JNICALL Prepared(JNIEnv *env, jobject thiz, jstring sourceStr) {
    LOGI("call jni prepared!");
    const char *source = env->GetStringUTFChars(sourceStr, 0);
    if (g_WlFFmpeg == NULL) {
        if (g_CallJava == NULL) {
            g_CallJava = new CallJava(g_JavaVm, env, thiz);
        }

        /*主线程中回调当前为加载缓冲状态,
         只有在JNI环境中创建了子线程，那么回调需要在子线程中回调，同时需要加载新的JNIenv环境
         */
        g_CallJava->OnCallLoad(MAIN_THREAD, true);

        g_PlayStatus = new WLPlayStatus();
        g_WlFFmpeg = new WLFFmpeg(g_PlayStatus, g_CallJava, source);
        g_WlFFmpeg->Prepared();
    }
    env->ReleaseStringUTFChars(sourceStr, source);
}

JNIEXPORT void JNICALL Start(JNIEnv *env, jobject thiz) {
    if (g_WlFFmpeg != NULL) {
        g_WlFFmpeg->Start();
    }
}

JNIEXPORT void JNICALL Pause(JNIEnv *env, jobject thiz) {
    if (g_WlFFmpeg != NULL) {
        g_WlFFmpeg->Pause();
    }
}

JNIEXPORT void JNICALL Resume(JNIEnv *env, jobject thiz) {
    if (g_WlFFmpeg != NULL) {
        g_WlFFmpeg->Resume();
    }
}

JNIEXPORT void JNICALL Stop(JNIEnv *env, jobject thiz) {
    if (g_IsExit) {//正在资源销毁的过程中，直接退出，不允许调用stop操作
        return;
    }

    g_IsExit = true;
    if (g_WlFFmpeg != NULL) {
        g_WlFFmpeg->Release();

        delete g_WlFFmpeg;
        g_WlFFmpeg = NULL;

        if (g_CallJava != NULL) {
            delete g_CallJava;
            g_CallJava = NULL;
        }

        if (g_PlayStatus != NULL) {
            delete g_PlayStatus;
            g_PlayStatus = NULL;
        }
    }
    g_IsExit = false;

    //若是播放结束，或者是播放过程中点击了Next按钮，才会先stop，然后播放下一个播放资源
    jclass jcz = env->GetObjectClass(thiz);
    jmethodID jmid_next = env->GetMethodID(jcz, "onCallNext", "()V");
    env->CallVoidMethod(thiz, jmid_next);
}

JNIEXPORT void JNICALL Seek(JNIEnv *env, jobject thiz, jint secds) {
    if (g_WlFFmpeg != NULL) {
        g_WlFFmpeg->Seek(secds);
    }
}

JNIEXPORT jint JNICALL Duration(JNIEnv *env, jobject thiz) {
    if (g_WlFFmpeg != NULL) {
        return g_WlFFmpeg->duration;
    }
    return 0;
}

JNIEXPORT void JNICALL Volume(JNIEnv *env, jobject thiz, jint percent) {
    if (g_WlFFmpeg != NULL) {
        g_WlFFmpeg->SetVolume(percent);
    }
}

JNIEXPORT void JNICALL Mute(JNIEnv *env, jobject thiz, jint mute) {
    if (g_WlFFmpeg != NULL) {
        g_WlFFmpeg->SetMute(mute);
    }
}

JNIEXPORT void JNICALL Pitch(JNIEnv *env, jobject thiz, jfloat pitch) {
    if (g_WlFFmpeg != NULL) {
        g_WlFFmpeg->SetPitch(pitch);
    }
}

JNIEXPORT void JNICALL Speed(JNIEnv *env, jobject thiz, jfloat speed) {
    if (g_WlFFmpeg != NULL) {
        g_WlFFmpeg->SetSpeed(speed);
    }
}

JNIEXPORT jint JNICALL Samplerate(JNIEnv *env, jobject thiz) {
    if (g_WlFFmpeg != NULL) {
        return g_WlFFmpeg->GetSampleRate();
    }
    return 0;
}

JNIEXPORT void JNICALL StartstopRecord(JNIEnv *env, jobject thiz, jboolean start) {
    if (g_WlFFmpeg != NULL) {
        g_WlFFmpeg->StartStopRecord(start);
    }
}

JNIEXPORT jboolean JNICALL
CutAudioPlay(JNIEnv *env, jobject thiz, jint start_time, jint end_time, jboolean show_pcm) {
    if (g_WlFFmpeg != NULL) {
        return g_WlFFmpeg->CutAudioPlay(start_time, end_time, show_pcm);
    }
    return false;
}

static JNINativeMethod gMethods[] = {
        {"_nativePrepared",        "(Ljava/lang/String;)V", (void *) Prepared},
        {"_nativeStart",           "()V",                   (void *) Start},
        {"_nativePause",           "()V",                   (void *) Pause},
        {"_nativeResume",          "()V",                   (void *) Resume},
        {"_nativeStop",            "()V",                   (void *) Stop},
        {"_nativeSeek",            "(I)V",                  (void *) Seek},
        {"_nativeDuration",        "()I",                   (void *) Duration},
        {"_nativeVolume",          "(I)V",                  (void *) Volume},
        {"_nativeMute",            "(I)V",                  (void *) Mute},
        {"_nativePitch",           "(F)V",                  (void *) Pitch},
        {"_nativeSpeed",           "(F)V",                 (void *) Speed},
        {"_nativeSamplerate",      "()I",                  (void *) Samplerate},
        {"_nativeStartstopRecord", "(Z)V",                 (void *) StartstopRecord},
        {"_nativeCutAudioPlay",    "(IIZ)Z",               (void *) CutAudioPlay},
};

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    g_JavaVm = vm;
    if (g_JavaVm->GetEnv((void **) (&env), JNI_VERSION_1_6) != JNI_OK) {
        LOGE("JNI_OnLoad GetEnv failed!");
        return -1;
    }
    assert(env != NULL);

    jclass clazz = env->FindClass(kClassPathName);
    if (clazz == NULL) {
        LOGE("class not found. %s", kClassPathName);
        return -1;
    }

    if (env->RegisterNatives(clazz, gMethods, sizeof(gMethods) / sizeof(gMethods[0])) < 0) {
        LOGE("RegisterNatives failed!");
        return -1;
    }

    DELETE_LOCAL_REF(env, clazz);
    return JNI_VERSION_1_6;
}