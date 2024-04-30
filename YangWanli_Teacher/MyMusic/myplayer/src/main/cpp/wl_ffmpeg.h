#ifndef MYPLAYER_WLFFMPEG_H_
#define MYPLAYER_WLFFMPEG_H_

#include "call_java.h"
#include "pthread.h"
#include "wl_audio.h"
#include "wl_video.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/time.h>
};

class WLFFmpeg {
public:
    WLFFmpeg(WLPlayStatus *playStatus, CallJava *calljava, const char *url);

    ~WLFFmpeg();

public:
    void prepared();

    void start();

    void pause();

    void resume();

    void seek(int64_t secds);

    void release();

    void setVolume(int percent);

    void setMute(int mute);

    void setPitch(float pitch);

    void setSpeed(float speed);

    int getSampleRate();

    void startStopRecord(bool start);

    bool cutAudioPlay(int start_time, int end_time, bool showPcm);

    void demuxFFmpegThread();

    void startFFmpegThread();

private:
    int getCodecContext(AVCodecParameters *codecPar, AVCodecContext** avCodecContext);

public:
    WLPlayStatus *playStatus = NULL;
    CallJava *callJava = NULL;
    char url[256] = {0};

    pthread_mutex_t init_mutex;
    pthread_mutex_t seek_mutex;
    pthread_t demuxThread;
    pthread_t startThread;

    AVFormatContext *pFormatCtx = NULL;
    WLAudio *pWLAudio = NULL;
    WLVideo* pWLVideo = NULL;

    bool isExit = false;
    int duration = 0;//媒体文件总时长
    bool supportMediaCodec = false;
};

#endif //MYPLAYER_WLFFMPEG_H_
