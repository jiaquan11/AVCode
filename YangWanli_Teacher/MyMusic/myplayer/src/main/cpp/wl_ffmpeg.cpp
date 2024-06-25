#include "wl_ffmpeg.h"

WLFFmpeg::WLFFmpeg(WLPlayStatus *play_status, CallJava *call_java, const char *url) {
    m_play_status = play_status;
    m_call_java_ = call_java;
    strcpy(m_url_, url);
    pthread_mutex_init(&m_init_mutex_, NULL);//初始化互斥锁,保证资源的初始化和释放是顺序的，安全的
    pthread_mutex_init(&m_seek_mutex_, NULL);
}

WLFFmpeg::~WLFFmpeg() {
    pthread_mutex_destroy(&m_init_mutex_);
    pthread_mutex_destroy(&m_seek_mutex_);
}

/**
 * 若在avformat_open_input调用时加载网络流很慢，导致阻塞卡死,
 * 这个回调函数中返回AVERROR_EOF会立即退出加载，返回失败
 */
int avformat_callback(void *ctx) {
    WLFFmpeg *wlfFmpeg = (WLFFmpeg *) (ctx);
    if (wlfFmpeg->m_play_status->m_is_exit) {
        return AVERROR_EOF;
    }
    return 0;
}

void WLFFmpeg::DemuxFFmpegThread() {
    LOGI("WLFFmpeg DemuxFFmpegThread in");
    pthread_mutex_lock(&m_init_mutex_);
    av_register_all();
    avformat_network_init();
    m_avformat_ctx_ = avformat_alloc_context();
    m_avformat_ctx_->interrupt_callback.callback = avformat_callback;//设置中断函数
    m_avformat_ctx_->interrupt_callback.opaque = this;
    if (avformat_open_input(&m_avformat_ctx_, m_url_, NULL, NULL) != 0) {
        if (LOG_DEBUG) {
            LOGE("can not open url: %s", m_url_);
            m_call_java_->OnCallError(CHILD_THREAD, 1001, "can not open url");
        }
        pthread_mutex_unlock(&m_init_mutex_);
        return;
    }
    if (avformat_find_stream_info(m_avformat_ctx_, NULL) < 0) {
        if (LOG_DEBUG) {
            LOGE("can not find streams from url: %s", m_url_);
            m_call_java_->OnCallError(CHILD_THREAD, 1002, "can not find streams from url");
        }
        pthread_mutex_unlock(&m_init_mutex_);
        return;
    }

    for (int i = 0; i < m_avformat_ctx_->nb_streams; ++i) {
        if (m_avformat_ctx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            if (m_wlaudio_ == NULL) {
                m_wlaudio_ = new WLAudio(m_avformat_ctx_->streams[i]->codecpar->sample_rate, m_play_status,m_call_java_);//创建音频播放类实例
                m_wlaudio_->m_stream_index = i;
                m_wlaudio_->m_codec_par = m_avformat_ctx_->streams[i]->codecpar;
                m_wlaudio_->time_base = m_avformat_ctx_->streams[i]->time_base;
                m_wlaudio_->m_duration = m_avformat_ctx_->duration / AV_TIME_BASE;//媒体文件总时长
                m_duration = m_wlaudio_->m_duration;
                m_call_java_->OnCallPcmInfo(CHILD_THREAD, m_wlaudio_->m_sample_rate, 16, 2);//上报音频采样率，采样位宽，和声道数信息
            }
        } else if (m_avformat_ctx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (m_wlvideo_ == NULL) {
                m_wlvideo_ = new WLVideo(m_play_status, m_call_java_);
                m_wlvideo_->m_stream_index = i;
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

    if (m_wlaudio_ != NULL) {
        _GetCodecContext(m_wlaudio_->m_codec_par, &m_wlaudio_->m_avcodec_ctx);
    }

    if (m_wlvideo_ != NULL) {
        _GetCodecContext(m_wlvideo_->m_codec_par, &m_wlvideo_->m_avcodec_ctx);
    }

    if (m_call_java_ != NULL) {
        m_call_java_->OnCallPrepared(CHILD_THREAD);
    }
    pthread_mutex_unlock(&m_init_mutex_);
    LOGI("WLFFmpeg DemuxFFmpegThread out");
}

void *_DemuxFFmpeg(void *arg) {
    WLFFmpeg *wlfFmpeg = (WLFFmpeg *) (arg);
    wlfFmpeg->DemuxFFmpegThread();

    /**
     * 使用return语句退出线程比使用pthread_exit()函数更简单和直观。
     * 一般情况下，如果只需要退出当前线程，
     * 而不需要精确的控制，使用return语句是更常见和推荐的做法。
     * 只有在需要在任意位置立即终止线程执行的特殊情况下，才需要使用pthread_exit()函数。
     *
     * pthread_exit()函数的作用是终止当前线程，但是不会终止整个进程。
     * 当线程调用pthread_exit()函数时，会将线程的退出状态返回给调用线程。
     * 如果线程是由main()函数创建的，那么线程的退出状态将返回给主进程。
     * 如果线程是由其他线程创建的，那么线程的退出状态将返回给创建线程。
     * */
//    pthread_exit(&wlfFmpeg->m_demux_thread_);
    return 0;
}

void WLFFmpeg::Prepare() {
    int ret = pthread_create(&m_demux_thread_, NULL, _DemuxFFmpeg, this);
    if (ret != 0) {
        if (LOG_DEBUG) {
            LOGE("WLFFmpeg pthread_create demux_thread_ failed");
            m_call_java_->OnCallError(MAIN_THREAD, 1000, "pthread_create demux_thread_ failed");
        }
    }
}

void WLFFmpeg::StartFFmpegThread() {
    LOGI("WLFFmpeg StartFFmpegThread in");
    if (m_wlaudio_ == NULL) {
        if (LOG_DEBUG) {
            LOGE("audio is NULL");
            m_call_java_->OnCallError(CHILD_THREAD, 1008, "audio is NULL");
        }
        return;
    }

    //要求必须要有视频流
//    if (m_wlvideo_ == NULL) {
//        return;
//    }

    bool support_mediacodec = false;
    m_wlvideo_->m_audio = m_wlaudio_;//将音频播放对象设置到视频播放对象中，用于获取音频参数进行音视频时间戳同步操作
    const char *codec_tag = (m_wlvideo_->m_avcodec_ctx->codec)->name;
    LOGI("WLFFmpeg start codecName: %s", codec_tag);
    if (support_mediacodec = m_call_java_->OnCallIsSupportMediaCodec(CHILD_THREAD, codec_tag)) {//回调Java函数，支持硬解，优先使用硬解
        LOGI("当前设备支持硬解码当前视频!!!");
        /**
         *对于硬解视频，必须传入的码流头是annexb格式，所以需要转换数据，添加annexb格式头
         */
        const AVBitStreamFilter * bs_filter = NULL;
        if (strcasecmp(codec_tag, "h264") == 0) {
            bs_filter = av_bsf_get_by_name("h264_mp4toannexb");
        } else if (strcasecmp(codec_tag, "hevc") == 0) {
            bs_filter = av_bsf_get_by_name("hevc_mp4toannexb");
        }
        if (bs_filter == NULL) {
            support_mediacodec = false;
            goto end;
        }
        if (av_bsf_alloc(bs_filter, &m_wlvideo_->m_abs_ctx) != 0) {
            support_mediacodec = false;
            goto end;
        }
        if (avcodec_parameters_copy(m_wlvideo_->m_abs_ctx->par_in, m_wlvideo_->m_codec_par) < 0) {
            support_mediacodec = false;
            av_bsf_free(&m_wlvideo_->m_abs_ctx);
            goto end;
        }
        if (av_bsf_init(m_wlvideo_->m_abs_ctx) != 0) {
            support_mediacodec = false;
            av_bsf_free(&m_wlvideo_->m_abs_ctx);
            goto end;
        }
        /**
         * 在大多数情况下，正确地设置AVBSFContext的time_base_in是必要的，
         * 因为比特流过滤器需要知道输入时间戳的时间基准，以便正确地处理时间戳。
         * 例如，如果你正在使用一个比特流过滤器来处理视频或音频数据，
         * 它需要知道输入数据的时间基准，以便正确地调整或生成输出数据的时间戳
         */
        m_wlvideo_->m_abs_ctx->time_base_in = m_wlvideo_->m_time_base;//时间基准
    }

    end:
    if (support_mediacodec) {
        m_wlvideo_->m_render_type = RENDER_MEDIACODEC;
        /**
         * 回调Java方法，传递ffmepg的extradata数据，用来初始化硬件解码器
         * 为了方便查看extradata数据，这里打印一下
         */
        LOGI("native onCallInitMediaCodec extradata size: %d", m_wlvideo_->m_avcodec_ctx->extradata_size);
        int size = m_wlvideo_->m_avcodec_ctx->extradata_size;
        char output[4];
        char buffer[1024] = {0};
        for (size_t i = 0; i < size; ++i) {
            sprintf(output, "%02X ", m_wlvideo_->m_avcodec_ctx->extradata[i]);
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
                                                           m_wlvideo_->m_avcodec_ctx->width, m_wlvideo_->m_avcodec_ctx->height,
                                                   m_wlvideo_->m_avcodec_ctx->extradata_size,m_wlvideo_->m_avcodec_ctx->extradata);
    }

    m_wlaudio_->Play();//开启音频播放，内部创建子线程用于获取缓冲区的pacekt，解码为pcm并给到opengles播放
    m_wlvideo_->Play();//开启视频播放,内部创建子线程用于获取缓冲区的pacekt,然后进行解码渲染

    LOGI("WLFFmpeg is start");
    while ((m_play_status != NULL) && !m_play_status->m_is_exit) {
        if (m_play_status->m_seek) {
            av_usleep(100 * 1000);
            LOGI("now is seek status, wait for a while");
            continue;
        }
        /**
         * 对于ape音频文件，一个音频packet可以解码为多个frame，因此需要减少缓冲区packet的个数
         * 避免seek时卡顿,但是对于一个packet对应一个frame的音频文件，这里要改为40
         * 为了避免seek时卡顿，这里控制一下读取包的速度，音频包缓冲队列存储的数据不宜过多，不往下读取
         */
        if (m_wlaudio_->m_packet_queue->GetQueueSize() > 40) {
            av_usleep(100 * 1000);
            continue;
        }

        AVPacket *av_packet = av_packet_alloc();
        pthread_mutex_lock(&m_seek_mutex_);
        int ret = av_read_frame(m_avformat_ctx_, av_packet);
        pthread_mutex_unlock(&m_seek_mutex_);
        if (ret == 0) {
            if (av_packet->stream_index == m_wlaudio_->m_stream_index) {
                m_wlaudio_->m_packet_queue->PutAVPacket(av_packet);
            } else if (av_packet->stream_index == m_wlvideo_->m_stream_index) {
                m_wlvideo_->m_packet_queue->PutAVPacket(av_packet);
            } else {//非音频packet
                av_packet_free(&av_packet);
            }
        } else {
            /**
             * 读取到文件尾，等待缓冲区中的数据消耗完
             */
            av_packet_free(&av_packet);
            while ((m_play_status != NULL) && !m_play_status->m_is_exit) {
                /**
                 * seek状态下，直接退出,不再等待缓冲区数据消耗完,会重新读取数据
                 */
                if (m_play_status->m_seek) {
                    break;
                }
                /**
                 * 等待音频和视频缓冲播放结束
                 */
                if ((m_wlaudio_->m_packet_queue->GetQueueSize() > 0) || (m_wlvideo_->m_packet_queue->GetQueueSize() > 0)) {
                    av_usleep(100 * 1000);
                    continue;
                }
                m_play_status->m_is_exit = true;
                m_is_play_end_ = true;
                LOGI("all data play end");
                break;
            }
        }
    }
    LOGI("demux is over");
    /**
     *只有整个播放完成，才回调状态
     *按上面的逻辑，是以音频播放结束为准
     */
    if (m_is_play_end_ && (m_call_java_ != NULL)) {
        LOGI("WLFFmpeg play call complete before");
        m_call_java_->OnCallComplete(CHILD_THREAD);
        LOGI("WLFFmpeg play call complete after");
    }
    LOGI("WLFFmpeg StartFFmpegThread out");
}

void *_StartFFmpeg(void *arg) {
    WLFFmpeg *wlfFmpeg = (WLFFmpeg *) (arg);
    wlfFmpeg->StartFFmpegThread();
//  pthread_exit(&wlfFmpeg->m_start_thread_);
    return 0;
}

void WLFFmpeg::Start() {
    int ret = pthread_create(&m_start_thread_, NULL, _StartFFmpeg, this);//开启一个子线程，用于读取流数据，并存放到缓存队列中
    if (ret != 0) {
        if (LOG_DEBUG) {
            LOGE("WLFFmpeg pthread_create m_start_thread_ failed");
            m_call_java_->OnCallError(MAIN_THREAD, 1007, "pthread_create m_start_thread_ failed");
        }
    }
}

void WLFFmpeg::Pause() {
    if (m_play_status != NULL) {
        m_play_status->m_pause = true;
    }
    if (m_wlaudio_ != NULL) {
        m_wlaudio_->Pause();//暂停音频opensl播放
    }
}

void WLFFmpeg::Resume() {
    if (m_play_status != NULL) {
        m_play_status->m_pause = false;
    }
    if (m_wlaudio_ != NULL) {
        m_wlaudio_->Resume();//恢复音频opensl播放
    }
}

void WLFFmpeg::Seek(int64_t secds) {
    LOGI("WLFFmpeg seek in secds: %lld", secds);
    if (m_duration <= 0) {
        LOGE("WLFFmpeg seek failed, m_duration <= 0");
        return;
    }
    if ((secds >= 0) && (secds <= m_duration)) {
        m_play_status->m_seek = true;//设置为seek状态
        pthread_mutex_lock(&m_seek_mutex_);
        int64_t ts = secds * AV_TIME_BASE;
        avformat_seek_file(m_avformat_ctx_, -1, INT64_MIN, ts, INT64_MAX, 0);//seek到指定的时间点，这里没有指定某个流进行seek，由ffmpeg内部去判断
        if (m_wlaudio_ != NULL) {
            m_wlaudio_->m_packet_queue->ClearAvPacket();
            m_wlaudio_->clock = 0;
            m_wlaudio_->last_time = 0;
            pthread_mutex_lock(&m_wlaudio_->m_codec_mutex);
            avcodec_flush_buffers(m_wlaudio_->m_avcodec_ctx);
            pthread_mutex_unlock(&m_wlaudio_->m_codec_mutex);
            LOGI("WLFFmpeg m_wlaudio_ seek!!! ");
        }
        if (m_wlvideo_ != NULL) {
            m_wlvideo_->m_packet_queue->ClearAvPacket();
            m_wlvideo_->m_clock = 0;
            pthread_mutex_lock(&m_wlvideo_->m_codec_mutex);
            avcodec_flush_buffers(m_wlvideo_->m_avcodec_ctx);
            pthread_mutex_unlock(&m_wlvideo_->m_codec_mutex);
            LOGI("WLFFmpeg m_wlvideo_ seek!!! ");
        }
        pthread_mutex_unlock(&m_seek_mutex_);
        m_play_status->m_seek = false;
        LOGI("WLFFmpeg seek end!");
    } else {
        LOGE("WLFFmpeg seek failed, secds is invalid");
    }
}

void WLFFmpeg::Release() {
    LOGI("WLFFmpeg release in");
    m_play_status->m_is_exit = true;

    void *thread_ret;
    int result = pthread_join(m_demux_thread_, &thread_ret);
    LOGI("m_demux_thread_ join result: %d", result);
    if (result != 0) {
        switch (result) {
            case ESRCH:
                LOGE("m_demux_thread_ pthread_join failed: Thread not found (ESRCH)");
                break;
            case EINVAL:
                LOGE("m_demux_thread_ pthread_join failed: Invalid thread or thread already detached (EINVAL)");
                break;
            case EDEADLK:
                LOGE("m_demux_thread_ pthread_join failed: Deadlock detected (EDEADLK)");
                break;
            default:
                LOGE("m_demux_thread_ pthread_join failed: Unknown error (%d)", result);
        }
        // Exit or perform additional actions if needed
        exit(EXIT_FAILURE);
    } else {
        LOGI("m_demux_thread_ Thread returned: %ld", (long)thread_ret);
    }
    result = pthread_join(m_start_thread_, &thread_ret);
    LOGI("m_start_thread_ join result: %d", result);
    if (result != 0) {
        switch (result) {
            case ESRCH:
                LOGE("m_start_thread_ pthread_join failed: Thread not found (ESRCH)");
                break;
            case EINVAL:
                LOGE("m_start_thread_ pthread_join failed: Invalid thread or thread already detached (EINVAL)");
                break;
            case EDEADLK:
                LOGE("m_start_thread_ pthread_join failed: Deadlock detected (EDEADLK)");
                break;
            default:
                LOGE("m_start_thread_ pthread_join failed: Unknown error (%d)", result);
        }
        // Exit or perform additional actions if needed
        exit(EXIT_FAILURE);
    } else {
        LOGI("m_start_thread_ Thread returned: %ld", (long)thread_ret);
    }

    pthread_mutex_lock(&m_init_mutex_);
    if (m_wlaudio_ != NULL) {
        m_wlaudio_->Release();
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
    if (m_avformat_ctx_ != NULL) {
        avformat_close_input(&m_avformat_ctx_);
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
        m_wlaudio_->SetVolume(percent);
    }
}

void WLFFmpeg::SetChannelType(int channel_type) {
    if (m_wlaudio_ != NULL) {
        m_wlaudio_->SetChannelType(channel_type);
    }
}

void WLFFmpeg::SetPitch(float pitch) {
    if (m_wlaudio_ != NULL) {
        m_wlaudio_->SetPitch(pitch);
    }
}

void WLFFmpeg::SetSpeed(float speed) {
    if (m_wlaudio_ != NULL) {
        m_wlaudio_->SetSpeed(speed);
    }
}

void WLFFmpeg::StartStopRecord(bool start) {
    if (m_wlaudio_ != NULL) {
        m_wlaudio_->StartStopRecord(start);
    }
}

bool WLFFmpeg::CutAudioPlay(int start_time, int end_time, bool show_pcm) {
    if ((start_time >= 0) && (end_time <= m_duration) && (start_time < end_time)) {//符合裁剪条件
        m_wlaudio_->m_is_cut = true;
        m_wlaudio_->end_time = end_time;
        m_wlaudio_->m_show_pcm = show_pcm;
        Seek(start_time);
        return true;
    } else {
        LOGE("cut audio play failed, start_time or end_time is invalid");
        m_wlaudio_->m_is_cut = false;
        return false;
    }
}

int WLFFmpeg::_GetCodecContext(AVCodecParameters *codec_par, AVCodecContext **av_codec_ctx) {
    AVCodec *av_codec = avcodec_find_decoder(codec_par->codec_id);
    if (!av_codec) {
        if (LOG_DEBUG) {
            LOGE("can not find deocder");
        }
        m_call_java_->OnCallError(CHILD_THREAD, 1003, "can not find deocder");
        pthread_mutex_unlock(&m_init_mutex_);
        return -1;
    }

    *av_codec_ctx = avcodec_alloc_context3(av_codec);
    if (!*av_codec_ctx) {
        if (LOG_DEBUG) {
            LOGE("can not alloc new decoder context");
        }
        m_call_java_->OnCallError(CHILD_THREAD, 1004, "can not alloc new decoder context");
        pthread_mutex_unlock(&m_init_mutex_);
        return -2;
    }

    if (avcodec_parameters_to_context(*av_codec_ctx, codec_par) < 0) {
        if (LOG_DEBUG) {
            LOGE("can not fill decoder context");
        }
        m_call_java_->OnCallError(CHILD_THREAD, 1005, "can not fill decoder context");
        pthread_mutex_unlock(&m_init_mutex_);
        return -3;
    }

    if (avcodec_open2(*av_codec_ctx, av_codec, 0) != 0) {
        if (LOG_DEBUG) {
            LOGE("can not open decoder");
        }
        m_call_java_->OnCallError(CHILD_THREAD, 1006, "can not open decoder");
        pthread_mutex_unlock(&m_init_mutex_);
        return -4;
    }
    LOGI("open decoder success!");
    return 0;
}
