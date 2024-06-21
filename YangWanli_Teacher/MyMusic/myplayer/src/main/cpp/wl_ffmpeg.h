#ifndef MYPLAYER_WLFFMPEG_H_
#define MYPLAYER_WLFFMPEG_H_

#include "pthread.h"
extern "C" {
    #include <libavformat/avformat.h>
    #include <libavutil/time.h>
};

#include "call_java.h"
#include "wl_audio.h"
#include "wl_video.h"

class WLFFmpeg {
public:
    WLFFmpeg(WLPlayStatus *play_status, CallJava *call_java, const char *url);

    ~WLFFmpeg();

public:
    void DemuxFFmpegThread();

    void Prepare();

    void StartFFmpegThread();

    void Start();

    void Pause();

    void Resume();

    void Seek(int64_t secds);

    void Release();

    void SetVolume(int percent);

    void SetChannelType(int channel_type);

    void SetPitch(float pitch);

    void SetSpeed(float speed);

    void StartStopRecord(bool flag);

    bool CutAudioPlay(int start_time, int end_time, bool show_pcm);

private:
    int _GetCodecContext(AVCodecParameters *codecPar, AVCodecContext** av_codec_ctx);

public:
    WLPlayStatus *m_play_status = NULL;
    int m_duration = 0;

private:
    CallJava *m_call_java_ = NULL;
    char m_url_[256] = {0};

    pthread_mutex_t m_init_mutex_;
    pthread_mutex_t m_seek_mutex_;
    pthread_t m_demux_thread_;
    pthread_t m_start_thread_;

    AVFormatContext *m_avformat_ctx_ = NULL;
    WLAudio* m_wlaudio_ = NULL;
    WLVideo* m_wlvideo_ = NULL;

    bool m_is_exit_ = false;
    bool m_support_mediacodec_ = false;
    bool m_is_play_end_ = false;
};

#endif //MYPLAYER_WLFFMPEG_H_
