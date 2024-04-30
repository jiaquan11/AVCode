#ifndef MYPLAYER_CALLJAVA_H_
#define MYPLAYER_CALLJAVA_H_

#include "jni.h"
#include "log/android_log.h"
#include <stdlib.h>
#include <string.h>

#define MAIN_THREAD 0
#define CHILD_THREAD 1

class CallJava {
public:
    CallJava(JavaVM *vm, JNIEnv *env, jobject obj);

    ~CallJava();

public:
    void OnCallPrepared(int type);

    void OnCallLoad(int type, bool load);

    void OnCallTimeInfo(int type, int curr, int total);

    void OnCallComplete(int type);

    void OnCallError(int type, int code, char *msg);

    void OnCallVolumeDB(int type, int db);

    void OnCallPcmInfo(int type, void *buffer, int size);

    void OnCallPcmRate(int type, int samplerate, int bit, int channels);

    void OnCallPcmToAAC(int type, void *buffer, int size);

    bool OnCallIsSupportVideo(int type, const char *ffcodecname);

    void OnCallinitMediaCodec(int type, const char *mime, int width, int height, int csd_size, uint8_t *csd);

    void OnCallRenderYUV(int type, int width, int linesize, int height, uint8_t *fy, uint8_t *fu, uint8_t *fv);

    void OnCallDecodeVPacket(int type, int datasize, uint8_t *data);

private:
    void CutAndCopyYuv(uint8_t *tempP[], uint8_t *srcfy, uint8_t *srcfu, uint8_t *srcfv, int linesize, int width, int height);

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

    uint8_t *pData[3] = {NULL};
    bool bHasAllocate = false;
};

#endif //MYPLAYER_CALLJAVA_H_
