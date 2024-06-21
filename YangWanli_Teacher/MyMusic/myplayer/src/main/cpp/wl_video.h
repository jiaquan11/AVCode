#ifndef MYPLAYER_WLVIDEO_H_
#define MYPLAYER_WLVIDEO_H_

#include <pthread.h>
extern "C" {
#include <libswscale/swscale.h>
#include "include/libavcodec/avcodec.h"
#include <libavutil/time.h>
#include <libavutil/imgutils.h>
};

#include "wl_queue.h"
#include "call_java.h"
#include "wl_audio.h"

#define CODEC_YUV 0
#define CODEC_MEDIACODEC 1

class WLVideo {
public:
    WLVideo(WLPlayStatus *play_status, CallJava *callJava);

    ~WLVideo();

public:
    void Play();

    void Release();

    double GetFrameDiffTime(AVFrame *avframe, AVPacket *avpacket);

    double GetDelayTime(double diff);

public:
    int m_stream_index = -1;
    AVCodecContext *m_avcodec_ctx = NULL;
    AVCodecParameters *m_codec_par = NULL;
    WLQueue *m_queue = NULL;
    WLPlayStatus *m_play_status = NULL;
    CallJava *m_call_java = NULL;
    AVRational m_time_base;

    pthread_mutex_t m_codec_mutex;
    pthread_t m_thread_play;

    WLAudio *m_audio = NULL;
    double m_clock = 0;
    double m_delay_time = 0;
    double m_default_delay_time = 0.04;

    int m_codec_type = CODEC_YUV;
    AVBSFContext *m_abs_ctx = NULL;
};

#endif //MYPLAYER_WLVIDEO_H_
