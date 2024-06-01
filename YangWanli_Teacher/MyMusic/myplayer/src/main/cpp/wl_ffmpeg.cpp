#include "wl_ffmpeg.h"
#include <iostream>
#include <iomanip>

WLFFmpeg::WLFFmpeg(WLPlayStatus *playStatus, CallJava *calljava, const char *url) {
    m_play_status = playStatus;
    m_call_java_ = calljava;
    strcpy(m_url_, url);
    m_is_exit_ = false;
    pthread_mutex_init(&m_init_mutex_, NULL);
    pthread_mutex_init(&m_seek_mutex_, NULL);
}

WLFFmpeg::~WLFFmpeg() {
    pthread_mutex_destroy(&m_init_mutex_);
    pthread_mutex_destroy(&m_seek_mutex_);
}

/*
 * 若在avformat_open_input调用时加载网络流很慢，导致阻塞卡死，这个回调函数中返回AVERROR_EOF
 * 会立即退出加载，返回失败
 * */
int avformat_callback(void *ctx) {
    WLFFmpeg *wlfFmpeg = (WLFFmpeg *) (ctx);
    if (wlfFmpeg->m_play_status->m_is_exit) {
        return AVERROR_EOF;
    }
    return 0;
}

void WLFFmpeg::DemuxFFmpegThread() {
    pthread_mutex_lock(&m_init_mutex_);

    av_register_all();
    avformat_network_init();

    m_avformat_ctx_ = avformat_alloc_context();
    m_avformat_ctx_->interrupt_callback.callback = avformat_callback;
    m_avformat_ctx_->interrupt_callback.opaque = this;
    if (avformat_open_input(&m_avformat_ctx_, m_url_, NULL, NULL) != 0) {
        if (LOG_DEBUG) {
            LOGE("can not open url: %s", m_url_);
            m_call_java_->OnCallError(CHILD_THREAD, 1001, "can not open url");
        }
        m_is_exit_ = true;
        pthread_mutex_unlock(&m_init_mutex_);
        return;
    }
    if (avformat_find_stream_info(m_avformat_ctx_, NULL) < 0) {
        if (LOG_DEBUG) {
            LOGE("can not find streams from url: %s", m_url_);
            m_call_java_->OnCallError(CHILD_THREAD, 1002, "can not find streams from url");
        }
        m_is_exit_ = true;
        pthread_mutex_unlock(&m_init_mutex_);
        return;
    }

    for (int i = 0; i < m_avformat_ctx_->nb_streams; ++i) {
        if (m_avformat_ctx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            if (m_wlaudio_ == NULL) {
                m_wlaudio_ = new WLAudio(m_play_status, m_avformat_ctx_->streams[i]->codecpar->sample_rate, m_call_java_);//创建音频播放类实例
                m_wlaudio_->streamIndex = i;
                m_wlaudio_->codecPar = m_avformat_ctx_->streams[i]->codecpar;
                m_wlaudio_->duration = m_avformat_ctx_->duration / AV_TIME_BASE;//媒体总时长，单位为秒
                m_wlaudio_->time_base = m_avformat_ctx_->streams[i]->time_base;
                m_duration = m_wlaudio_->duration;

                m_call_java_->OnCallPcmRate(CHILD_THREAD, m_wlaudio_->sample_Rate, 16, 2);//上报音频采样率，采样位宽，和声道数信息
            }
        } else if (m_avformat_ctx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (m_wlvideo_ == NULL) {
                m_wlvideo_ = new WLVideo(m_play_status, m_call_java_);//创建视频播放类实例
                m_wlvideo_->m_streamIndex = i;
                m_wlvideo_->m_codec_par = m_avformat_ctx_->streams[i]->codecpar;
                m_wlvideo_->m_time_base = m_avformat_ctx_->streams[i]->time_base;

                int num = m_avformat_ctx_->streams[i]->avg_frame_rate.num;
                int den = m_avformat_ctx_->streams[i]->avg_frame_rate.den;
                if ((num != 0) && (den != 0)) {//获取到平均帧率值
                    int fps = num / den;//比如25/1
                    m_wlvideo_->m_default_delay_time = 1.0 / fps;//根据帧率值计算得到每一帧的播放延时
                    LOGI("fps %d, defaultDelayTime: %lf", fps, m_wlvideo_->m_default_delay_time);
                }
            }
        }
    }

    //打开ffmpeg音频解码器
    if (m_wlaudio_ != NULL) {
        _GetCodecContext(m_wlaudio_->codecPar, &m_wlaudio_->avCodecContext);
    }

    //打开ffmpeg视频解码器
    if (m_wlvideo_ != NULL) {
        _GetCodecContext(m_wlvideo_->m_codec_par, &m_wlvideo_->m_avcodec_context);
    }

    if (m_call_java_ != NULL) {
        if ((m_play_status != NULL) && !m_play_status->m_is_exit) {
            m_call_java_->OnCallPrepared(CHILD_THREAD);//回调已准备好资源
        } else {
            m_is_exit_ = true;
        }
    }
    pthread_mutex_unlock(&m_init_mutex_);
}

void *demuxFFmpeg(void *data) {
    WLFFmpeg *wlfFmpeg = (WLFFmpeg *) (data);
    wlfFmpeg->DemuxFFmpegThread();

    /*
     * 使用return语句退出线程比使用pthread_exit()函数更简单和直观。
     * 一般情况下，如果只需要退出当前线程，
     * 而不需要精确的控制，使用return语句是更常见和推荐的做法。
     * 只有在需要在任意位置立即终止线程执行的特殊情况下，才需要使用pthread_exit()函数。
     * */
//    pthread_exit(&wlfFmpeg->m_demux_thread_);
    return 0;
}

void WLFFmpeg::Prepared() {
    pthread_create(&m_demux_thread_, NULL, demuxFFmpeg, this);
}

void WLFFmpeg::StartFFmpegThread() {
    if (m_wlaudio_ == NULL) {
        if (LOG_DEBUG) {
            LOGE("audio is NULL");
            m_call_java_->OnCallError(CHILD_THREAD, 1007, "audio is NULL");
        }
        return;
    }

    if (m_wlvideo_ == NULL) {//目前要求必须要有视频流
        return;
    }

    m_support_mediacodec_ = false;
    m_wlvideo_->m_audio = m_wlaudio_;//将音频播放对象设置到视频播放对象中，用于获取音频参数进行音视频时间戳同步操作
    const char *codec_tag = (m_wlvideo_->m_avcodec_context->codec)->name;
    LOGI("WLFFmpeg start codecName: %s", codec_tag);
    if (m_support_mediacodec_ = m_call_java_->OnCallIsSupportVideo(CHILD_THREAD, codec_tag)) {//回调Java函数，支持硬解，优先使用硬解
        LOGI("当前设备支持硬解码当前视频!!!");
        /*
         * 对于硬解视频，必须传入的码流头是annexb格式，所以需要转换数据，添加annexb格式头
         * */
        const AVBitStreamFilter * bsFilter = NULL;
        if (strcasecmp(codec_tag, "h264") == 0) {
            bsFilter = av_bsf_get_by_name("h264_mp4toannexb");
        } else if (strcasecmp(codec_tag, "hevc") == 0) {
            bsFilter = av_bsf_get_by_name("hevc_mp4toannexb");
        }
        if (bsFilter == NULL) {
            goto end;
        }
        if (av_bsf_alloc(bsFilter, &m_wlvideo_->m_abs_ctx) != 0) {
            m_support_mediacodec_ = false;
            goto end;
        }
        if (avcodec_parameters_copy(m_wlvideo_->m_abs_ctx->par_in, m_wlvideo_->m_codec_par) < 0) {
            m_support_mediacodec_ = false;
            av_bsf_free(&m_wlvideo_->m_abs_ctx);
            m_wlvideo_->m_abs_ctx = NULL;
            goto end;
        }
        if (av_bsf_init(m_wlvideo_->m_abs_ctx) != 0) {
            m_support_mediacodec_ = false;
            av_bsf_free(&m_wlvideo_->m_abs_ctx);
            m_wlvideo_->m_abs_ctx = NULL;
            goto end;
        }
        m_wlvideo_->m_abs_ctx->time_base_in = m_wlvideo_->m_time_base;//时间基准
    }

    end:
    if (m_support_mediacodec_) {
        m_wlvideo_->m_codec_type = CODEC_MEDIACODEC;
        /*
         * 回调Java方法，传递ffmepg的extradata数据，用来初始化硬件解码器，
         * */
//        for (int i = 0;i < m_wlvideo_->avCodecContext->extradata_size; i++) {
//            LOGI("%02X", m_wlvideo_->avCodecContext->extradata[i]);
//        }
        LOGI("native onCallInitMediaCodec extradata size: %d", m_wlvideo_->m_avcodec_context->extradata_size);
        int size = m_wlvideo_->m_avcodec_context->extradata_size;
        char output[4];
        char buffer[1024] = {0};
        for (size_t i = 0; i < size; ++i) {
            sprintf(output, "%02X ", m_wlvideo_->m_avcodec_context->extradata[i]);
            strcat(buffer, output);
            if ((i + 1) % 16 == 0) {
                strcat(buffer, "\n");
                LOGD("%s", buffer);
                memset(buffer, 0, sizeof(buffer));
            }
        }
        if ((size % 16) != 0) {
            strcat(buffer, "\n");
            LOGD("%s", buffer);
        }

        m_wlvideo_->m_call_java->OnCallInitMediaCodec(CHILD_THREAD,codec_tag,
                                                   m_wlvideo_->m_avcodec_context->width, m_wlvideo_->m_avcodec_context->height,
                                                   m_wlvideo_->m_avcodec_context->extradata_size,m_wlvideo_->m_avcodec_context->extradata);
    }

    m_wlaudio_->play();//开启音频播放，内部创建子线程用于获取缓冲区的pacekt，解码为pcm并给到opengles播放
    m_wlvideo_->Play();//开启视频播放,内部创建子线程用于获取缓冲区的pacekt,然后进行解码渲染

    LOGI("WLFFmpeg is start");
    int count = 0;
    while ((m_play_status != NULL) && !m_play_status->m_is_exit) {
        if (m_play_status->m_seek) {
            av_usleep(100 * 1000);
            LOGI("now is seek continue");
            continue;
        }
        /*对于ape音频文件，一个音频packet可以解码为多个frame，因此需要减少缓冲区packet的个数，
         * 避免seek时卡顿,但是对于一个packet对应一个frame的音频文件，这里要改为40
         */
        if (m_wlaudio_->queue->GetQueueSize() > 40) {//这里控制一下读取包的速度，音频包缓冲队列存储的数据不宜过多，不往下读取
            av_usleep(100 * 1000);//100毫秒
            continue;
        }

        AVPacket *avPacket = av_packet_alloc();
        pthread_mutex_lock(&m_seek_mutex_);
        int ret = av_read_frame(m_avformat_ctx_, avPacket);
        pthread_mutex_unlock(&m_seek_mutex_);
        if (ret == 0) {
            if (avPacket->stream_index == m_wlaudio_->streamIndex) {
                count++;
                if (LOG_DEBUG) {
//                    LOGI("read audio the packet: %d", count);
                }
                m_wlaudio_->queue->PutAVPacket(avPacket);
            } else if (avPacket->stream_index == m_wlvideo_->m_streamIndex) {
                m_wlvideo_->m_queue->PutAVPacket(avPacket);
            } else {//非音频packet
                av_packet_free(&avPacket);
                av_free(avPacket);
            }
        } else {//读取到文件尾，等待缓冲区中的数据消耗完
            av_packet_free(&avPacket);
            av_free(avPacket);
            while ((m_play_status != NULL) && !m_play_status->m_is_exit) {
                if (m_wlaudio_->queue->GetQueueSize() > 0) {//音频缓冲区中的packet未消耗完，则处于线程延迟中，等待音频播放线程消耗
                    av_usleep(100 * 1000);//100毫秒
                    continue;
                } else {//音频缓冲区中的packet已消耗完
                    if (!m_play_status->m_seek) {
                        av_usleep(100 * 1000);
                        m_play_status->m_is_exit = true;
                        LOGI("playStatus m_is_exit_ set true");
                    }
                    break;
                }
            }
        }
    }

    /*回调整个播放已完成
     按上面的逻辑，是以音频播放结束为准
     */
    if (m_call_java_ != NULL) {
        m_call_java_->OnCallComplete(CHILD_THREAD);
    }

    m_is_exit_ = true;
    if (LOG_DEBUG) {
        LOGI("get packet is over");
    }
}

void *startFFmpeg(void *data) {
    WLFFmpeg *wlfFmpeg = (WLFFmpeg *) (data);
    wlfFmpeg->StartFFmpegThread();

    /*
     * 使用return语句退出线程比使用pthread_exit()函数更简单和直观。
     * 一般情况下，如果只需要退出当前线程，
     * 而不需要精确的控制，使用return语句是更常见和推荐的做法。
     * 只有在需要在任意位置立即终止线程执行的特殊情况下，才需要使用pthread_exit()函数。
     * */
//    pthread_exit(&wlfFmpeg->m_demux_thread_);
    return 0;
}

void WLFFmpeg::Start() {
    pthread_create(&m_start_thread_, NULL, startFFmpeg, this);//开启一个子线程，用于读取流数据，并存放到缓存队列中
}

void WLFFmpeg::Pause() {
    if (m_play_status != NULL) {
        m_play_status->m_pause = true;
    }

    if (m_wlaudio_ != NULL) {
        m_wlaudio_->pause();
    }
}

void WLFFmpeg::Resume() {
    if (m_play_status != NULL) {
        m_play_status->m_pause = false;
    }

    if (m_wlaudio_ != NULL) {
        m_wlaudio_->resume();
    }
}

void WLFFmpeg::Seek(int64_t secds) {
    LOGI("WLFFmpeg seek secds: %lld", secds);
    if (m_duration <= 0) {
        return;
    }
    if ((secds >= 0) && (secds <= m_duration)) {
        m_play_status->m_seek = true;//设置为seek状态
        pthread_mutex_lock(&m_seek_mutex_);
        int64_t rel = secds * AV_TIME_BASE;
        avformat_seek_file(m_avformat_ctx_, -1, INT64_MIN, rel, INT64_MAX, 0);//seek到指定的时间点，这里没有指定某个流进行seek，由ffmpeg内部去判断
        if (m_wlaudio_ != NULL) {
            m_wlaudio_->queue->ClearAvPacket();
            m_wlaudio_->clock = 0;
            m_wlaudio_->last_time = 0;
            pthread_mutex_lock(&m_wlaudio_->codecMutex);
            avcodec_flush_buffers(m_wlaudio_->avCodecContext);//清空解码器内部缓冲
            pthread_mutex_unlock(&m_wlaudio_->codecMutex);
            LOGI("WLFFmpeg m_wlaudio_ seek!!! ");
        }

        if (m_wlvideo_ != NULL) {
            m_wlvideo_->m_queue->ClearAvPacket();
            m_wlvideo_->m_clock = 0;
            pthread_mutex_lock(&m_wlvideo_->m_codec_mutex);
            avcodec_flush_buffers(m_wlvideo_->m_avcodec_context);
            pthread_mutex_unlock(&m_wlvideo_->m_codec_mutex);
            LOGI("WLFFmpeg m_wlvideo_ seek!!! ");
        }
        pthread_mutex_unlock(&m_seek_mutex_);
        m_play_status->m_seek = false;
        LOGI("WLFFmpeg seek end!");
    }
}

void WLFFmpeg::Release() {
    LOGI("WLFFmpeg release in");
    m_play_status->m_is_exit = true;

    pthread_join(m_demux_thread_, NULL);//等待子线程结束
    pthread_join(m_start_thread_, NULL);//等待子线程结束

    pthread_mutex_lock(&m_init_mutex_);
    int sleepCount = 0;
    while (!m_is_exit_) {//若播放子线程仍然没有退出，则延迟等待10s
        if (sleepCount > 1000) {
            m_is_exit_ = true;
        }
        if (LOG_DEBUG) {
            LOGI("wait ffmpeg exit %d", sleepCount);
        }
        sleepCount++;
        av_usleep(1000 * 10);//10ms
    }

    if (m_wlaudio_ != NULL) {
        m_wlaudio_->release();
        delete m_wlaudio_;
        m_wlaudio_ = NULL;
        LOGI("WLFFmpeg release m_wlaudio_");
    }

    if (m_wlvideo_ != NULL) {
        m_wlvideo_->Release();
        delete m_wlvideo_;
        m_wlvideo_ = NULL;
        LOGI("WLFFmpeg release m_wlvideo_");
    }

    LOGI("WLFFmpeg release m_avformat_ctx_");
    if (m_avformat_ctx_ != NULL) {
        avformat_close_input(&m_avformat_ctx_);
        avformat_free_context(m_avformat_ctx_);
        m_avformat_ctx_ = NULL;
    }

    if (m_play_status != NULL) {
        m_play_status = NULL;
    }
    if (m_call_java_ != NULL) {
        m_call_java_ = NULL;
    }
    pthread_mutex_unlock(&m_init_mutex_);
    LOGI("WLFFmpeg release end");
}

void WLFFmpeg::SetVolume(int percent) {
    if (m_wlaudio_ != NULL) {
        m_wlaudio_->setVolume(percent);
    }
}

void WLFFmpeg::SetMute(int mute) {
    if (m_wlaudio_ != NULL) {
        m_wlaudio_->setMute(mute);
    }
}

void WLFFmpeg::SetPitch(float pitch) {
    if (m_wlaudio_ != NULL) {
        m_wlaudio_->setPitch(pitch);
    }
}

void WLFFmpeg::SetSpeed(float speed) {
    if (m_wlaudio_ != NULL) {
        m_wlaudio_->setSpeed(speed);
    }
}

int WLFFmpeg::GetSampleRate() {
    if (m_wlaudio_ != NULL) {
        return m_wlaudio_->avCodecContext->sample_rate;
    }
    return 0;
}

void WLFFmpeg::StartStopRecord(bool start) {
    if (m_wlaudio_ != NULL) {
        m_wlaudio_->startStopRecord(start);
    }
}

bool WLFFmpeg::CutAudioPlay(int start_time, int end_time, bool showPcm) {
    if ((start_time >= 0) && (end_time <= m_duration) && (start_time < end_time)) {//符合裁剪条件
        m_wlaudio_->isCut = true;
        m_wlaudio_->end_time = end_time;
        m_wlaudio_->showPcm = showPcm;

        Seek(start_time);
        return true;
    }
    return false;
}

int WLFFmpeg::_GetCodecContext(AVCodecParameters *codecPar, AVCodecContext **avCodecContext) {
    AVCodec *avCodec = avcodec_find_decoder(codecPar->codec_id);
    if (!avCodec) {
        if (LOG_DEBUG) {
            LOGE("can not find deocder");
        }
        m_call_java_->OnCallError(CHILD_THREAD, 1003, "can not find deocder");
        m_is_exit_ = true;
        pthread_mutex_unlock(&m_init_mutex_);
        return -1;
    }

    *avCodecContext = avcodec_alloc_context3(avCodec);
    if (!*avCodecContext) {
        if (LOG_DEBUG) {
            LOGE("can not alloc new decoderCtx");
        }
        m_call_java_->OnCallError(CHILD_THREAD, 1004, "can not alloc new decoderCtx");
        m_is_exit_ = true;
        pthread_mutex_unlock(&m_init_mutex_);
        return -1;
    }

    if (avcodec_parameters_to_context(*avCodecContext, codecPar) < 0) {
        if (LOG_DEBUG) {
            LOGE("can not fill decoderCtx");
        }
        m_call_java_->OnCallError(CHILD_THREAD, 1005, "can not fill decoderCtx");
        m_is_exit_ = true;
        pthread_mutex_unlock(&m_init_mutex_);
        return -1;
    }

    if (avcodec_open2(*avCodecContext, avCodec, 0) != 0) {
        if (LOG_DEBUG) {
            LOGE("can not open audio decoder");
        }
        m_call_java_->OnCallError(CHILD_THREAD, 1006, "can not open audio decoder");
        m_is_exit_ = true;
        pthread_mutex_unlock(&m_init_mutex_);
        return -1;
    }
    LOGI("decoder open success!");
    return 0;
}
