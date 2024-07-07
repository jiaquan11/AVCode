#ifndef MYPLAYER_WLAUDIO_H_
#define MYPLAYER_WLAUDIO_H_

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <SLES/OpenSLES_Android.h>
#include <SLES/OpenSLES.h>
#include "soundtouch/include/SoundTouch.h"
extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libswresample/swresample.h>
    #include <libavutil/time.h>
}

#include "wl_queue.h"
#include "wl_play_status.h"
#include "call_java.h"
#include "wl_buffer_queue.h"
#include "wl_pcm_bean.h"

using namespace soundtouch;
class WLAudio {
public:
    WLAudio(int sample_rate, WLPlayStatus *play_status, CallJava *call_java);

    ~WLAudio();

public:
    void InitOpenSLES();

    void Play();

    void Pause();

    void Resume();

    void Stop();

    void Release();

    void SetVolume(int percent);

    void SetChannelType(int mute);

    void SetPitch(float pitch);

    void SetSpeed(float speed);

    int GetPCMDB(char *pcmdata, size_t pcmsize);

    int GetSoundTouchData();

    void StartStopRecord(bool start);

private:
    int _ResampleAudio(void **pcmbuf);

    SLuint32 _GetCurrentSampleRateForOpenSLES(int sample_rate);

public:
    CallJava *m_call_java = NULL;
    WLPlayStatus *m_play_status = NULL;
    pthread_mutex_t m_codec_mutex;
    int m_stream_index = -1;
    AVCodecParameters *m_codec_par = NULL;
    AVCodecContext *m_avcodec_ctx = NULL;
    int m_sample_rate = 0;
    int m_duration = 0;
    AVRational m_time_base;
    double m_clock = 0;
    double m_last_time = 0;
    bool m_is_cut = false;
    int m_end_time = 0;
    bool m_show_pcm = false;

    WLQueue *m_packet_queue = NULL;
    WLBufferQueue *m_buffer_queue;
    const int kDefaultPcmSize = 4096;

    SAMPLETYPE *m_sample_buffer = NULL;
    SLAndroidSimpleBufferQueueItf m_pcm_buffer_queue = NULL;
    bool m_is_record_pcm = false;

private:
    pthread_t m_play_thread_;
    pthread_t m_pcm_report_callback_thread_;
    // 引擎接口
    SLObjectItf m_engine_object_ = NULL;
    SLEngineItf m_engine_engine_ = NULL;
    //混音器
    SLObjectItf m_output_mix_object_ = NULL;
    SLEnvironmentalReverbItf m_output_mix_environmental_reverb_ = NULL;
    SLEnvironmentalReverbSettings m_reverbsettings_;
    //pcm播放器
    SLObjectItf m_pcm_player_object_ = NULL;
    SLPlayItf m_pcm_player_play_ = NULL;
    SLVolumeItf m_pcm_player_volume_ = NULL;
    SLMuteSoloItf m_pcm_player_channel_ = NULL;//声道操作

    int m_volume_percent_ = 100;
    int m_channel_type_ = 2;
    AVPacket *m_avpacket_ = NULL;
    AVFrame *m_avframe_ = NULL;
    uint8_t *m_convert_buffer_ = NULL;
    int m_data_size_ = 0;
    bool m_need_receive_samples_ = true;
    int m_nb_ = 0;
    SoundTouch *m_soundtouch_ = NULL;
    float m_pitch_ = 1.0f;
    float m_speed_ = 1.0f;
    double m_now_time_ = 0;
    SwrContext *m_swr_ctx_ = NULL;
    bool m_need_get_packet_ = true;
};

#endif //MYPLAYER_WLAUDIO_H_
