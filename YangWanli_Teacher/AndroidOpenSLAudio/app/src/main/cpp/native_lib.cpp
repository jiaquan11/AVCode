#include <jni.h>
#include <string>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include "log/android_log.h"

#define DELETE_LOCAL_REF(env, obj)  if(obj!=NULL){env->DeleteLocalRef(obj);obj=NULL;}
static const char *const kClassPathName = "com/jiaquan/androidopenslaudio/MainActivity";

// 引擎接口
SLObjectItf g_EngineObject = NULL;
SLEngineItf g_EngineEngine = NULL;

//混音器
SLObjectItf g_OutputMixObject = NULL;
SLEnvironmentalReverbItf g_OutputMixEnvironmentalReverb = NULL;
SLEnvironmentalReverbSettings g_ReverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;//走廊环境音混响

SLObjectItf g_PcmPlayerObject = NULL;
SLPlayItf g_PcmPlayerPlay = NULL;
SLVolumeItf g_PcmPlayerVolume = NULL;

//缓冲器队列接口
SLAndroidSimpleBufferQueueItf g_PcmBufferQueue = NULL;

FILE *pcmFile = NULL;
void *buffer = NULL;
uint8_t *out_buffer = NULL;

int getPcmData(void **pcm) {
    int size = 0;
    while (!feof(pcmFile)) {
        size = fread(out_buffer, 1, 44100 * 2 * 2, pcmFile);
        if (size == 0) {
            LOGE("Read end");
            break;
        } else {
            LOGI("reading");
        }
        *pcm = out_buffer;
        break;
    }
    return size;
}

//给到OpenSLES注册的回调函数，会由OpenSLES主动调用，直接将pcm数据放入OpenSLES中的缓冲队列中进行播放
void pcmBufferCallBack(SLAndroidSimpleBufferQueueItf bf, void *context) {
    int size = getPcmData(&buffer);
    LOGI("size is: %d", size);
    if (buffer != NULL) {
        SLresult result = (*g_PcmBufferQueue)->Enqueue(g_PcmBufferQueue, buffer, size);
    }
}

JNIEXPORT void JNICALL PlayPcm(JNIEnv *env, jobject thiz, jstring url_str) {
    const char *url = env->GetStringUTFChars(url_str, 0);
    pcmFile = fopen(url, "r");//打开pcm文件
    if (pcmFile == NULL) {
        return;
    }

    out_buffer = (uint8_t *) malloc(44100 * 2 * 2);//分配了1s的音频数据内存

    SLresult result;
    //1.创建接口对象(根据engineObject接口类对象来创建引擎对象,后面的操作都根据这个引擎对象创建相应的操作接口)
    slCreateEngine(&g_EngineObject, 0, 0, 0, 0, 0);
    (*g_EngineObject)->Realize(g_EngineObject, SL_BOOLEAN_FALSE);
    (*g_EngineObject)->GetInterface(g_EngineObject, SL_IID_ENGINE, &g_EngineEngine);

    //2.设置混音器
    const SLInterfaceID mids[1] = {SL_IID_ENVIRONMENTALREVERB};//混响音效类型
    const SLboolean mreq[1] = {SL_BOOLEAN_TRUE};//设置使能混响音效
    result = (*g_EngineEngine)->CreateOutputMix(g_EngineEngine, &g_OutputMixObject, 1, mids,
                                                mreq);//通过引擎创建混音器
    (void) result;
    result = (*g_OutputMixObject)->Realize(g_OutputMixObject, SL_BOOLEAN_FALSE);//实现混音器实例
    result = (*g_OutputMixObject)->GetInterface(g_OutputMixObject,
                                                SL_IID_ENVIRONMENTALREVERB,//指定混响音效类型
                                                &g_OutputMixEnvironmentalReverb);//通过混音器设备得到混响音效的实例
    if (SL_RESULT_SUCCESS == result) {
        result = (*g_OutputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
                g_OutputMixEnvironmentalReverb, &g_ReverbSettings);//设置某种指定的混响音效，比如走廊混响
        (void) result;
    }

    //3.创建播放器(录音器)
    SLDataLocator_AndroidSimpleBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                            2};//指定了两个buffer队列
    SLDataFormat_PCM pcm = {
            SL_DATAFORMAT_PCM,
            2,
            SL_SAMPLINGRATE_44_1,
            SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
            SL_BYTEORDER_LITTLEENDIAN};
    SLDataSource slDataSource = {&android_queue, &pcm};
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, g_OutputMixObject};
    SLDataSink audioSink = {&outputMix, NULL};
    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND,
                                  SL_IID_VOLUME};//指定使能缓存队列，音效和音量操作的三种操作接口
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};

    //根据引擎创建音频播放器实例
    result = (*g_EngineEngine)->CreateAudioPlayer(g_EngineEngine, &g_PcmPlayerObject, &slDataSource,
                                                  &audioSink, 3, ids, req);
    // 初始化播放器
    (*g_PcmPlayerObject)->Realize(g_PcmPlayerObject, SL_BOOLEAN_FALSE);
    //得到接口后调用  获取Player接口
    (*g_PcmPlayerObject)->GetInterface(g_PcmPlayerObject, SL_IID_PLAY,
                                       &g_PcmPlayerPlay);//根据音频播放器实例获取到音频播放接口

    //4.设置缓存队列和回调函数
    (*g_PcmPlayerObject)->GetInterface(g_PcmPlayerObject, SL_IID_BUFFERQUEUE,
                                       &g_PcmBufferQueue);//根据音频播放器实例获取到音频缓存队列的接口
    (*g_PcmBufferQueue)->RegisterCallback(g_PcmBufferQueue, pcmBufferCallBack, NULL);//注册回调函数

    //获取音量接口
    (*g_PcmPlayerObject)->GetInterface(g_PcmPlayerObject, SL_IID_VOLUME, &g_PcmPlayerVolume);

    //5.设置播放状态
    (*g_PcmPlayerPlay)->SetPlayState(g_PcmPlayerPlay, SL_PLAYSTATE_PLAYING);

    //6.启动回调函数
    pcmBufferCallBack(g_PcmBufferQueue, NULL);

    env->ReleaseStringUTFChars(url_str, url);
}

static JNINativeMethod gMethods[] = {
        {"_nativePlayPcm", "(Ljava/lang/String;)V", (void *) PlayPcm},
};

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    if (vm->GetEnv((void **) (&env), JNI_VERSION_1_6) != JNI_OK) {
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