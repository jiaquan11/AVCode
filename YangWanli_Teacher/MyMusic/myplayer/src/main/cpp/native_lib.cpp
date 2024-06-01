#include <pthread.h>
#include <jni.h>
#include <string>

extern "C" {
    #include "include/libavformat/avformat.h"
    #include "include/libavcodec/avcodec.h"
    #include "include/libavutil/avutil.h"
}

#include "log/android_log.h"
#include "call_java.h"
#include "wl_ffmpeg.h"

#define DELETE_LOCAL_REF(env, obj)  if(obj!=NULL){env->DeleteLocalRef(obj);obj=NULL;}
static const char *const kClassPathName = "com/jiaquan/myplayer/player/WLPlayer";

//全局变量
JavaVM *g_java_vm = NULL;
CallJava *g_call_java = NULL;
WLFFmpeg *g_wl_ffmpeg = NULL;
WLPlayStatus *g_play_status = NULL;
bool g_is_exit = false;

JNIEXPORT void JNICALL Prepared(JNIEnv *env, jobject thiz, jstring source_jstr) {
    LOGI("call jni prepared!");
    const char *source = env->GetStringUTFChars(source_jstr, 0);
    if (g_wl_ffmpeg == NULL) {
        if (g_call_java == NULL) {
            g_call_java = new CallJava(g_java_vm, env, thiz);
        }
        g_call_java->OnCallLoad(MAIN_THREAD, true);

        g_play_status = new WLPlayStatus();
        g_wl_ffmpeg = new WLFFmpeg(g_play_status, g_call_java, source);
        g_wl_ffmpeg->Prepared();
    }
    env->ReleaseStringUTFChars(source_jstr, source);
}

JNIEXPORT void JNICALL Start(JNIEnv *env, jobject thiz) {
    if (g_wl_ffmpeg != NULL) {
        g_wl_ffmpeg->Start();
    }
}

JNIEXPORT void JNICALL Pause(JNIEnv *env, jobject thiz) {
    if (g_wl_ffmpeg != NULL) {
        g_wl_ffmpeg->Pause();
    }
}

JNIEXPORT void JNICALL Resume(JNIEnv *env, jobject thiz) {
    if (g_wl_ffmpeg != NULL) {
        g_wl_ffmpeg->Resume();
    }
}

JNIEXPORT void JNICALL Stop(JNIEnv *env, jobject thiz) {
    if (g_is_exit) {//正在资源销毁的过程中，直接退出，不允许调用stop操作
        return;
    }

    g_is_exit = true;
    if (g_wl_ffmpeg != NULL) {
        g_wl_ffmpeg->Release();

        delete g_wl_ffmpeg;
        g_wl_ffmpeg = NULL;

        if (g_play_status != NULL) {
            delete g_play_status;
            g_play_status = NULL;
        }

        if (g_call_java != NULL) {
            delete g_call_java;
            g_call_java = NULL;
        }
    }
    g_is_exit = false;

    /**
     * 正常播放结束，或者停止播放，或者播放过程中点击了Next按钮，都会先执行stop操作
     * 然后回调onCallNext,onCallNext函数中会先检查是否有下一个播放资源，如果有，则会调用prepared函数，开始新的播放
     */
    jclass jcz = env->GetObjectClass(thiz);
    jmethodID jmid_next = env->GetMethodID(jcz, "onCallNext", "()V");
    env->CallVoidMethod(thiz, jmid_next);
}

JNIEXPORT void JNICALL Seek(JNIEnv *env, jobject thiz, jint secds) {
    if (g_wl_ffmpeg != NULL) {
        g_wl_ffmpeg->Seek(secds);
    }
}

JNIEXPORT jint JNICALL Duration(JNIEnv *env, jobject thiz) {
    if (g_wl_ffmpeg != NULL) {
        return g_wl_ffmpeg->m_duration;
    }
    return 0;
}

JNIEXPORT void JNICALL Volume(JNIEnv *env, jobject thiz, jint percent) {
    if (g_wl_ffmpeg != NULL) {
        g_wl_ffmpeg->SetVolume(percent);
    }
}

JNIEXPORT void JNICALL ChannelType(JNIEnv *env, jobject thiz, jint channel_type) {
    if (g_wl_ffmpeg != NULL) {
        g_wl_ffmpeg->SetChannelType(channel_type);
    }
}

JNIEXPORT void JNICALL Pitch(JNIEnv *env, jobject thiz, jfloat pitch) {
    if (g_wl_ffmpeg != NULL) {
        g_wl_ffmpeg->SetPitch(pitch);
    }
}

JNIEXPORT void JNICALL Speed(JNIEnv *env, jobject thiz, jfloat speed) {
    if (g_wl_ffmpeg != NULL) {
        g_wl_ffmpeg->SetSpeed(speed);
    }
}

JNIEXPORT jint JNICALL Samplerate(JNIEnv *env, jobject thiz) {
    if (g_wl_ffmpeg != NULL) {
        return g_wl_ffmpeg->GetSampleRate();
    }
    return 0;
}

JNIEXPORT void JNICALL StartstopRecord(JNIEnv *env, jobject thiz, jboolean start) {
    if (g_wl_ffmpeg != NULL) {
        g_wl_ffmpeg->StartStopRecord(start);
    }
}

JNIEXPORT jboolean JNICALL
CutAudioPlay(JNIEnv *env, jobject thiz, jint start_time, jint end_time, jboolean show_pcm) {
    if (g_wl_ffmpeg != NULL) {
        return g_wl_ffmpeg->CutAudioPlay(start_time, end_time, show_pcm);
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
         {"_nativeChannelType",     "(I)V",                  (void *) ChannelType},
         {"_nativePitch",           "(F)V",                  (void *) Pitch},
        {"_nativeSpeed",           "(F)V",                  (void *) Speed},
        {"_nativeSamplerate",      "()I",                   (void *) Samplerate},
        {"_nativeStartstopRecord", "(Z)V",                  (void *) StartstopRecord},
        {"_nativeCutAudioPlay",    "(IIZ)Z",                (void *) CutAudioPlay},
};

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    g_java_vm = vm;
    if (g_java_vm->GetEnv((void **) (&env), JNI_VERSION_1_6) != JNI_OK) {
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

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *vm, void *reserved) {
    LOGI("JNI_OnUnload in");
    JNIEnv *env;
    if (vm->GetEnv((void **) (&env), JNI_VERSION_1_6) != JNI_OK) {
        LOGE("JNI_OnUnload GetEnv failed!");
        return;
    }
    assert(env != NULL);

    if (g_java_vm != NULL) {
        g_java_vm = NULL;
    }

    if (g_call_java != NULL) {
        delete g_call_java;
        g_call_java = NULL;
    }

    if (g_wl_ffmpeg != NULL) {
        delete g_wl_ffmpeg;
        g_wl_ffmpeg = NULL;
    }

    if (g_play_status != NULL) {
        delete g_play_status;
        g_play_status = NULL;
    }
    LOGI("JNI_OnUnload out");
}