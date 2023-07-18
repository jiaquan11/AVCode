#ifndef _CALLJAVA_H_
#define _CALLJAVA_H_

#include "jni.h"
#include "log/androidLog.h"
#include <stdlib.h>
#include <string.h>

#define MAIN_THREAD 0
#define CHILD_THREAD 1

class CallJava {
public:
    CallJava(JavaVM *vm, JNIEnv *env, jobject obj);

    ~CallJava();

public:
    void onCallPrepared(int type);

    void onCallLoad(int type, bool load);

    void onCallTimeInfo(int type, int curr, int total);

    void onCallError(int type, int code, char *msg);

    void onCallComplete(int type);

    void onCallVolumeDB(int type, int db);

    void onCallPcmToAAC(int type, void *buffer, int size);

    void onCallPcmInfo(int type, void *buffer, int size);

    void onCallPcmRate(int type, int samplerate, int bit, int channels);

    void onCallRenderYUV(int type, int width, int linesize, int height, uint8_t *fy, uint8_t *fu, uint8_t *fv);

    bool onCallIsSupportVideo(int type, const char* ffcodecname);

    void onCallinitMediaCodec(int type, const char* mime, int width, int height, int csd0_size, int csd1_size, uint8_t* csd_0, uint8_t* csd_1);

    void onCallDecodeVPacket(int type, int datasize, uint8_t* data);

private:
    void cutAndCopyYuv(uint8_t* tempP[], uint8_t *srcfy, uint8_t *srcfu, uint8_t *srcfv, int linesize, int width, int height);

private:
    JavaVM *javaVm = NULL;
    JNIEnv *jniEnv = NULL;
    jobject jobj = NULL;

    jmethodID jmid_prepared;
    jmethodID jmid_load;
    jmethodID jmid_timeinfo;
    jmethodID jmid_complete;
    jmethodID jmid_error;
    jmethodID jmid_volumeDB;
    jmethodID jmid_pcminfo;
    jmethodID jmid_pcmrate;
    jmethodID jmid_pcmtoaac;
    jmethodID jmid_supportvideo;
    jmethodID jmid_initmediacodec;
    jmethodID jmid_renderyuv;
    jmethodID jmid_decodeVPacket;

    uint8_t* pData[3] = {NULL};
    bool bHasAllocate = false;
};
#endif
