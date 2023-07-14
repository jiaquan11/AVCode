#include <jni.h>
#include <string>
#include "log/androidLog.h"
#include "CallJava.h"
#include "WLFFmpeg.h"
#include <pthread.h>

extern "C" {
#include "include/libavformat/avformat.h"
#include "include/libavcodec/avcodec.h"
#include "include/libavutil/avutil.h"
}

JavaVM *javaVM = NULL;
CallJava *callJava = NULL;
WLFFmpeg *wlfFmpeg = NULL;
WLPlayStatus *playStatus = NULL;

bool isExit = true;

pthread_t thread_start;

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    jint result = -1;
    javaVM = vm;
    JNIEnv *env;
    if (vm->GetEnv((void **) (&env), JNI_VERSION_1_4) != JNI_OK) {
        return result;
    }
    return JNI_VERSION_1_4;
}

extern "C" JNIEXPORT void JNICALL Java_com_jiaquan_myplayer_player_WLPlayer__1prepared(JNIEnv *env, jobject thiz, jstring sourceStr) {
    LOGI("call jni prepared!");
    const char *source = env->GetStringUTFChars(sourceStr, 0);
    if (wlfFmpeg == NULL) {
        if (callJava == NULL) {
            callJava = new CallJava(javaVM, env, thiz);
        }

        callJava->onCallLoad(MAIN_THREAD, true);

        playStatus = new WLPlayStatus();
        wlfFmpeg = new WLFFmpeg(playStatus, callJava, source);
        wlfFmpeg->prepared();
    }
    env->ReleaseStringUTFChars(sourceStr, source);
}

void *startCallBack(void *data) {
    WLFFmpeg *wlfFmpeg = (WLFFmpeg *) (data);
    wlfFmpeg->start();
//    pthread_exit(&thread_start);
    return 0;
}

extern "C" JNIEXPORT void JNICALL Java_com_jiaquan_myplayer_player_WLPlayer__1start(JNIEnv *env, jobject thiz) {
    if (wlfFmpeg != NULL) {
//        fFmpeg->start();
        pthread_create(&thread_start, NULL, startCallBack, wlfFmpeg);
    }
}

extern "C" JNIEXPORT void JNICALL Java_com_jiaquan_myplayer_player_WLPlayer__1pause(JNIEnv *env, jobject thiz) {
    if (wlfFmpeg != NULL) {
        wlfFmpeg->pause();
    }
}

extern "C" JNIEXPORT void JNICALL Java_com_jiaquan_myplayer_player_WLPlayer__1resume(JNIEnv *env, jobject thiz) {
    if (wlfFmpeg != NULL) {
        wlfFmpeg->resume();
    }
}

extern "C" JNIEXPORT void JNICALL Java_com_jiaquan_myplayer_player_WLPlayer__1stop(JNIEnv *env, jobject thiz) {
    if (!isExit) {
        return;
    }

    jclass jcz = env->GetObjectClass(thiz);
    jmethodID jmid_next = env->GetMethodID(jcz, "onCallNext", "()V");

    isExit = false;

    if (wlfFmpeg != NULL) {
        wlfFmpeg->release();

        pthread_join(thread_start, NULL);

        delete wlfFmpeg;
        wlfFmpeg = NULL;

        if (callJava != NULL) {
            delete callJava;
            callJava = NULL;
        }

        if (playStatus != NULL) {
            delete playStatus;
            playStatus = NULL;
        }
    }
    isExit = true;
    env->CallVoidMethod(thiz, jmid_next);
}

extern "C" JNIEXPORT void JNICALL Java_com_jiaquan_myplayer_player_WLPlayer__1seek(JNIEnv *env, jobject thiz, jint secds) {
    if (wlfFmpeg != NULL) {
        wlfFmpeg->seek(secds);
    }
}

extern "C" JNIEXPORT jint JNICALL Java_com_jiaquan_myplayer_player_WLPlayer__1duration(JNIEnv *env, jobject thiz) {
    if (wlfFmpeg != NULL) {
        return wlfFmpeg->duration;
    }
    return 0;
}

extern "C" JNIEXPORT void JNICALL Java_com_jiaquan_myplayer_player_WLPlayer__1volume(JNIEnv *env, jobject thiz, jint percent) {
    if (wlfFmpeg != NULL) {
        wlfFmpeg->setVolume(percent);
    }
}

extern "C" JNIEXPORT void JNICALL Java_com_jiaquan_myplayer_player_WLPlayer__1mute(JNIEnv *env, jobject thiz, jint mute) {
    if (wlfFmpeg != NULL) {
        wlfFmpeg->setMute(mute);
    }
}

extern "C" JNIEXPORT void JNICALL Java_com_jiaquan_myplayer_player_WLPlayer__1pitch(JNIEnv *env, jobject thiz, jfloat pitch) {
    if (wlfFmpeg != NULL) {
        wlfFmpeg->setPitch(pitch);
    }
}

extern "C" JNIEXPORT void JNICALL Java_com_jiaquan_myplayer_player_WLPlayer__1speed(JNIEnv *env, jobject thiz, jfloat speed) {
    if (wlfFmpeg != NULL) {
        wlfFmpeg->setSpeed(speed);
    }
}

extern "C" JNIEXPORT jint JNICALL Java_com_jiaquan_myplayer_player_WLPlayer__1samplerate(JNIEnv *env, jobject thiz) {
    if (wlfFmpeg != NULL) {
        return wlfFmpeg->getSampleRate();
    }
    return 0;
}

extern "C" JNIEXPORT void JNICALL Java_com_jiaquan_myplayer_player_WLPlayer__1startstopRecord(JNIEnv *env, jobject thiz, jboolean start) {
    if (wlfFmpeg != NULL) {
        wlfFmpeg->startStopRecord(start);
    }
}

extern "C" JNIEXPORT jboolean JNICALL Java_com_jiaquan_myplayer_player_WLPlayer__1cutAudioPlay(JNIEnv *env, jobject thiz, jint start_time, jint end_time, jboolean show_pcm) {
    if (wlfFmpeg != NULL){
        return wlfFmpeg->cutAudioPlay(start_time, end_time, show_pcm);
    }
    return false;
}