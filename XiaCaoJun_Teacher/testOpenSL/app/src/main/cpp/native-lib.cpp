#include <jni.h>
#include <string>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <android/log.h>

#define LOG_TAG "testsl"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

void PcmCall(SLAndroidSimpleBufferQueueItf bf, void *context) {
    LOGI("PcmCall");
    static FILE *fp = NULL;
    static char *buf = NULL;
    if (!buf) {
        buf = new char[1024 * 1024 * 10];
    }
    char *path = "/storage/emulated/0/Pictures/the_girl.pcm";
    if (!fp) {
        fp = fopen(path, "rb");
        if (fp == NULL) {
            LOGE("open pcm file failed! %s", path);
            return;
        } else {
            LOGI("open pcm file success!");
        }
    }
    if (!fp) return;
    if (feof(fp) == 0) {
        int len = fread(buf, 1, 1024, fp);
        if (len > 0) {
            (*bf)->Enqueue(bf, buf, len);
        }
    }
}

static SLObjectItf engineSL = NULL;
SLEngineItf CreateSL() {
    SLresult ret;
    SLEngineItf en;
    ret = slCreateEngine(&engineSL, 0, 0, 0, 0, 0);//创建引擎
    if (ret != SL_RESULT_SUCCESS) return NULL;
    ret = (*engineSL)->Realize(engineSL, SL_BOOLEAN_FALSE);//实例化
    if (ret != SL_RESULT_SUCCESS) return NULL;
    ret = (*engineSL)->GetInterface(engineSL, SL_IID_ENGINE, &en);//获取引擎接口
    if (ret != SL_RESULT_SUCCESS) return NULL;
    return en;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_testopensl_MainActivity_stringFromJNI(JNIEnv *env, jobject /* this */) {
    std::string hello = "Hello from C++";
    //1 创建引擎
    SLEngineItf eng = CreateSL();
    if (eng) {
        LOGI("CreateSL success!");
    } else {
        LOGE("CreateSL failed!");
    }

    //创建混音器
    SLObjectItf mix = NULL;
    SLresult ret = 0;
    ret = (*eng)->CreateOutputMix(eng, &mix, 0, 0, 0);//创建混音器
    if (ret != SL_RESULT_SUCCESS) {
        LOGE("CreateOutputMix failed!");
    }
    ret = (*mix)->Realize(mix, SL_BOOLEAN_FALSE);//实例化混音器
    if (ret != SL_RESULT_SUCCESS) {
        LOGE("mix Realize failed!");
    }

    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, mix};
    SLDataSink audioSink = {&outputMix, 0};

    //3.配置音频信息
    //缓冲队列
    SLDataLocator_AndroidSimpleBufferQueue que = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 10};
    //音频格式
    SLDataFormat_PCM pcm = {
            SL_DATAFORMAT_PCM,
            2,//声道数
            SL_SAMPLINGRATE_44_1,
            SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
            SL_BYTEORDER_LITTLEENDIAN//字节序，小端
    };
    SLDataSource ds = {&que, &pcm};

    //4.创建播放器
    SLObjectItf player = NULL;
    SLPlayItf iplayer = NULL;
    SLAndroidSimpleBufferQueueItf pcmQue = NULL;
    const SLInterfaceID ids[] = {SL_IID_BUFFERQUEUE};
    const SLboolean req[] = {SL_BOOLEAN_TRUE};
    (*eng)->CreateAudioPlayer(eng, &player, &ds, &audioSink, sizeof(ids) / sizeof(SLInterfaceID), ids, req);//创建音频播放器
    if (ret != SL_RESULT_SUCCESS) {
        LOGE("CreateAudioPlayer failed!");
    } else {
        LOGI("CreateAudioPlayer success!");
    }

    (*player)->Realize(player, SL_BOOLEAN_FALSE);//音频播放器实例化

    //获取Player接口
    ret = (*player)->GetInterface(player, SL_IID_PLAY, &iplayer);//获取音频播放器的播放接口实例
    if (ret != SL_RESULT_SUCCESS) {
        LOGE("player GetInterface SL_IID_PLAY failed!");
    } else {
        LOGI("player GetInterface SL_IID_PLAY success!");
    }
    ret = (*player)->GetInterface(player, SL_IID_BUFFERQUEUE, &pcmQue);//获取音频播放器的缓冲队列
    if (ret != SL_RESULT_SUCCESS) {
        LOGE("player GetInterface SL_IID_BUFFERQUEUE failed");
    } else {
        LOGI("player GetInterface SL_IID_BUFFERQUEUE success");
    }

    //设置回调函数，播放队列空调用
    (*pcmQue)->RegisterCallback(pcmQue, PcmCall, 0);

    //设置播放状态
    (*iplayer)->SetPlayState(iplayer, SL_PLAYSTATE_PLAYING);

    //启动队列回调
    (*pcmQue)->Enqueue(pcmQue, "", 1);
    return env->NewStringUTF(hello.c_str());
}
