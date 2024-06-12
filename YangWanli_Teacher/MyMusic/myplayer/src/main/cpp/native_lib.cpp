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
    LOGI("native jni prepared in, g_wl_ffmpeg: %p", g_wl_ffmpeg);
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
    LOGI("native jni prepared out!");
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
    LOGI("native Stop in g_is_exit %d", g_is_exit);
    if (g_is_exit) {//正在资源销毁的过程中，直接退出，不允许调用stop操作
        LOGE("native Stop is exiting, can not call stop again,");
        return;
    }

    g_is_exit = true;
    if (g_wl_ffmpeg != NULL) {
        LOGI("release g_wl_ffmpeg before");
        g_wl_ffmpeg->Release();
        delete g_wl_ffmpeg;
        g_wl_ffmpeg = NULL;
        LOGI("release g_wl_ffmpeg over");
        if (g_play_status != NULL) {
            delete g_play_status;
            g_play_status = NULL;
            LOGI("release g_play_status over");
        }

        if (g_call_java != NULL) {
            delete g_call_java;
            g_call_java = NULL;
            LOGI("release g_call_java over");
        }
    }
    g_is_exit = false;
    LOGI("native Stop out");
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