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

    void OnCallInitMediaCodec(int type, const char *mime, int width, int height, int csd_size, uint8_t *csd);

    void OnCallRenderYUV(int type, int width, int linesize, int height, uint8_t *fy, uint8_t *fu, uint8_t *fv);

    void OnCallDecodeVPacket(int type, int datasize, uint8_t *data);

private:
    void _CutAndCopyYuv(uint8_t* data[], uint8_t *srcfy, uint8_t *srcfu, uint8_t *srcfv, int linesize, int width, int height);

private:
    JavaVM *m_java_vm_ = NULL;
    JNIEnv *m_jni_env_ = NULL;
    jobject m_jobj_ = NULL;

    jmethodID m_jmid_prepared_;
    jmethodID m_jmid_load_;
    jmethodID m_jmid_timeinfo_;
    jmethodID m_jmid_complete_;
    jmethodID m_jmid_error_;
    jmethodID m_jmid_volumedb_;
    jmethodID m_jmid_pcminfo_;
    jmethodID m_jmid_pcmrate_;
    jmethodID m_jmid_pcmtoaac_;
    jmethodID m_jmid_support_video_;
    jmethodID m_jmid_init_mediacodec_;
    jmethodID m_jmid_render_yuv_;
    jmethodID m_jmid_decode_vpacket_;

    uint8_t *m_data_[3] = {NULL};
    bool m_has_allocate_ = false;
};

#endif //MYPLAYER_CALLJAVA_H_
