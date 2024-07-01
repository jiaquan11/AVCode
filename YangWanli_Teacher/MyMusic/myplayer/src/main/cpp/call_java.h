#ifndef MYPLAYER_CALL_JAVA_H_
#define MYPLAYER_CALL_JAVA_H_

#include "jni.h"
#include <stdlib.h>
#include <string.h>

#include "log/android_log.h"

#define MAIN_THREAD 0
#define CHILD_THREAD 1

class CallJava {
public:
    CallJava(JavaVM *vm, JNIEnv *env, jobject obj);

    ~CallJava();

public:
    void OnCallLoad(int type, bool load);

    void OnCallPrepared(int type);

    void OnCallTimeInfo(int type, int curr, int total);

    void OnCallPcmInfo(int type, int samplerate, int bit, int channels);

    void OnCallPcmData(int type, void *buffer, int size);

    void OnCallVolumeDB(int type, int db);

    bool OnCallIsSupportMediaCodec(int type, const char *codec_tag);

    void OnCallInitMediaCodec(int type, const char *codec_tag, int width, int height);

    void OnCallDecodeVPacket(int type, uint8_t *data, int data_size, double pts_secds);

    void OnCallRenderYUV(int type, int width, int height, int linesize, uint8_t *y_data, uint8_t *u_data, uint8_t *v_data);

    void OnCallComplete(int type);

    void OnCallError(int type, int code, char *msg);

    void OnCallPcmToAAC(int type, void *buffer, int size);

private:
    void _CutAndCopyYuv(uint8_t* data[], uint8_t *srcy, uint8_t *srcu, uint8_t *srcv, int width, int height, int linesize);

private:
    JavaVM *m_java_vm_ = NULL;
    JNIEnv *m_jni_env_ = NULL;
    jobject m_jobj_ = NULL;

    jmethodID m_jmid_load_;
    jmethodID m_jmid_prepared_;
    jmethodID m_jmid_timeinfo_;
    jmethodID m_jmid_pcm_info_;
    jmethodID m_jmid_pcm_data_;
    jmethodID m_jmid_volumedb_;
    jmethodID m_jmid_is_support_mediacodec_;
    jmethodID m_jmid_init_mediacodec_;
    jmethodID m_jmid_decode_vpacket_;
    jmethodID m_jmid_render_yuv_;
    jmethodID m_jmid_complete_;
    jmethodID m_jmid_error_;
    jmethodID m_jmid_pcm_to_aac_;

    uint8_t *m_data_[3] = {NULL};
    bool m_has_allocate_ = false;
};

#endif //MYPLAYER_CALLJAVA_H_
