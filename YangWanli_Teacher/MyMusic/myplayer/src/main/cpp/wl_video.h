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

#define RENDER_YUV 0
#define RENDER_MEDIACODEC 1

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
    WLPlayStatus *m_play_status = NULL;
    CallJava *m_call_java = NULL;
    int m_render_type = RENDER_YUV;
    WLAudio *m_audio = NULL;
    int m_stream_index = -1;
    AVCodecContext *m_avcodec_ctx = NULL;
    AVCodecParameters *m_codec_par = NULL;
    AVBSFContext *m_abs_ctx = NULL;
    WLQueue *m_packet_queue = NULL;
    AVRational m_time_base;
    pthread_mutex_t m_codec_mutex;
    double m_clock = 0;
    double m_default_delay_time = 0.04;

private:
    pthread_t m_play_thread_;
    double m_delay_time_ = 0;
    double m_last_audio_clock_ = 0;
};

#endif //MYPLAYER_WLVIDEO_H_
