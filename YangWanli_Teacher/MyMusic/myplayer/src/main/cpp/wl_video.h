#ifndef MYPLAYER_WLVIDEO_H_
#define MYPLAYER_WLVIDEO_H_

#include "wl_queue.h"
#include "call_java.h"
#include <pthread.h>
#include "wl_audio.h"

#define CODEC_YUV 0
#define CODEC_MEDIACODEC 1

extern "C" {
#include <libswscale/swscale.h>
#include "include/libavcodec/avcodec.h"
#include <libavutil/time.h>
#include <libavutil/imgutils.h>
};

class WLVideo {
public:
    WLVideo(WLPlayStatus *playStatus, CallJava *callJava);

    ~WLVideo();

public:
    void play();

    void release();

    double getFrameDiffTime(AVFrame *avFrame, AVPacket *avPacket);

    double getDelayTime(double diff);

public:
    int streamIndex = -1;
    AVCodecContext *avCodecContext = NULL;
    AVCodecParameters *codecPar = NULL;
    WLQueue *queue = NULL;
    WLPlayStatus *playStatus = NULL;
    CallJava *callJava = NULL;
    AVRational time_base;

    pthread_mutex_t codecMutex;
    pthread_t thread_play;

    WLAudio *audio = NULL;
    double clock = 0;
    double delayTime = 0;
    double defaultDelayTime = 0.04;

    int codectype = CODEC_YUV;
    AVBSFContext *abs_ctx = NULL;
};

#endif //MYPLAYER_WLVIDEO_H_