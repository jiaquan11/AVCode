#ifndef MYPLAYER_WLVIDEO_H_
#define MYPLAYER_WLVIDEO_H_

#include <pthread.h>
extern "C" {
#include <libswscale/swscale.h>
#include "include/libavcodec/avcodec.h"
#include <libavutil/time.h>
#include <libavutil/imgutils.h>
};

#include "wl_packet_queue.h"
#include "wl_frame_queue.h"
#include "wl_link_order_queue.h"
#include "call_java.h"
#include "wl_audio.h"

#define RENDER_YUV 0
#define RENDER_MEDIACODEC 1

class WLVideo {
public:
    WLVideo(WLPlayStatus *play_status, CallJava *call_java);

    ~WLVideo();

public:
    void Decode();

    void Play();

    void Release();

    double GetFrameDiffTime(int pts_ms);

    double GetFrameDiffTime(AVFrame *avframe, AVPacket *avpacket);

    double GetDelayTime(double diff_secds);

    void Get264Params(AVCodecContext *avctx);

    void Get265Params(AVCodecContext *avctx);

public:
    WLPlayStatus *m_play_status = NULL;
    CallJava *m_call_java = NULL;
    int m_render_type = RENDER_YUV;
    WLAudio *m_audio = NULL;
    int m_stream_index = -1;
    AVCodecContext *m_avcodec_ctx = NULL;
    AVCodecParameters *m_codec_par = NULL;
    AVBSFContext *m_abs_ctx = NULL;
    AVPacket *m_avpacket = NULL;
    AVFrame *m_avframe = NULL;
    AVFrame *m_temp_avframe = NULL;
    uint8_t *m_scale_buffer = NULL;
    SwsContext *m_sws_ctx = NULL;
    WLPacketQueue *m_packet_queue = NULL;
    WLFrameQueue* m_frame_queue = NULL;
    WLLinkOrderQueue *m_pts_queue = NULL;
    AVRational m_time_base;
    int m_duration = 0;
    pthread_mutex_t m_codec_mutex;
    double m_clock = 0;
    double m_last_time = 0;
    double m_default_delay_time = 0.04;
    int m_max_ref_frames = 0;
    int m_b_frames = 0;
    bool m_read_packet_finished = false;
    bool m_is_video_data_end = false;

private:
    pthread_t m_play_thread_;
    pthread_t m_decode_thread_;
    double m_delay_time_ = 0;
    double m_last_audio_clock_ = 0;
    double m_last_video_clock_ = 0;

    static const int kMaxPpsLen = 2000;
    char *m_vps_ = NULL;
    int m_vps_len_ = 0;
    char *m_sps_ = NULL;
    int m_sps_len_ = 0;
    char *m_pps_ = NULL;
    int m_pps_len = 0;
};

#endif //MYPLAYER_WLVIDEO_H_
