#include "wl_audio.h"

WLAudio::WLAudio(int sample_rate, WLPlayStatus *play_status, CallJava *call_java) {
    m_play_status = play_status;
    m_sample_rate = sample_rate;
    m_call_java = call_java;
    m_is_cut = false;
    this->end_time = 0;
    this->showPcm = false;

    m_queue = new WLQueue(m_play_status);

    m_buffer_ = (uint8_t *) (av_malloc(sample_rate * 2 * 2));//分配了一秒的音频pcm数据内存

    //音频变速变调功能
    sampleBuffer = static_cast<SAMPLETYPE *>(malloc(sample_rate * 2 * 2));//分配了一秒的音频pcm数据内存
    m_soundtouch_ = new SoundTouch();
    m_soundtouch_->setSampleRate(sample_rate);
    m_soundtouch_->setChannels(2);
    m_soundtouch_->setPitch(pitch);
    m_soundtouch_->setTempo(speed);
//    outFile = fopen("/sdcard/testziliao/outAudio.pcm", "wb");
    LOGI("WLAudio construct pitch: %f speed: %f soundTouch:%p", pitch, speed, m_soundtouch_);
    pthread_mutex_init(&m_codec_mutex, NULL);
}

WLAudio::~WLAudio() {
    pthread_mutex_destroy(&m_codec_mutex);
}

//给到OpenSLES注册的回调函数，会由OpenSLES主动调用，直接将pcm数据放入OpenSLES中的缓冲队列中进行播放
void _PcmBufferCallBack(SLAndroidSimpleBufferQueueItf bf, void *context) {
    WLAudio *wlAudio = (WLAudio *) (context);
    if (wlAudio != NULL) {
        int bufferSize = wlAudio->GetSoundTouchData();//返回的是当前音频包解码后的PCM的采样点数(单个)
        if (bufferSize > 0) {
            wlAudio->clock += (bufferSize * 4) / ((double) (wlAudio->m_sample_rate * 2 * 2));//累加一下播放一段pcm所耗费的时间
//            LOGI("pcmBufferCallBack audio clock: %lf", wlAudio->clock);
            if (wlAudio->clock - wlAudio->last_time >= 0.1) {//100毫秒上报一次当前的音频播放时间戳
                wlAudio->last_time = wlAudio->clock;
                wlAudio->m_call_java->OnCallTimeInfo(CHILD_THREAD, wlAudio->clock, wlAudio->m_duration);
            }

            wlAudio->m_buffer_queue->PutBuffer(wlAudio->sampleBuffer, bufferSize * 4);//将解码的Pcm数据放入缓冲区，用于另外一个线程获取用于上报

            wlAudio->m_call_java->OnCallVolumeDB(CHILD_THREAD, wlAudio->GetPCMDB(reinterpret_cast<char *>(wlAudio->sampleBuffer), bufferSize * 4));//上报音频的分贝值

            (*wlAudio->pcmBufferQueue)->Enqueue(wlAudio->pcmBufferQueue, wlAudio->sampleBuffer, bufferSize * 2 * 2);//将pcm数据丢入opensles队列进行播放
            if (wlAudio->m_is_cut) {
                if (wlAudio->clock > wlAudio->end_time) {
                    wlAudio->m_play_status->m_is_exit = true;
                    LOGI("裁剪退出...");
                }
            }
        }
    }
}

void WLAudio::InitOpenSLES() {
    LOGI("initOpenSLES in");
    SLresult result;

    //1.创建接口对象(根据engineObject接口类对象来创建引擎对象,后面的操作都根据这个引擎对象创建相应的操作接口)
    slCreateEngine(&m_engine_object_, 0, 0, 0, 0, 0);
    (*m_engine_object_)->Realize(m_engine_object_, SL_BOOLEAN_FALSE);
    (*m_engine_object_)->GetInterface(m_engine_object_, SL_IID_ENGINE, &m_engine_engine_);

    //2.设置混音器
    const SLInterfaceID mids[1] = {SL_IID_ENVIRONMENTALREVERB};//设置使能混响音效
    const SLboolean mreq[1] = {SL_BOOLEAN_TRUE};
    result = (*m_engine_engine_)->CreateOutputMix(m_engine_engine_, &m_output_mix_object_, 1, mids, mreq);//通过引擎创建混音器
    (void) result;
    result = (*m_output_mix_object_)->Realize(m_output_mix_object_, SL_BOOLEAN_FALSE);//实现混音器实例
    (void) result;
    result = (*m_output_mix_object_)->GetInterface(m_output_mix_object_, SL_IID_ENVIRONMENTALREVERB,//指定混响音效类型
                                              &m_output_mix_environmental_reverb_);//通过混音器设备得到混响音效的实例
    if (SL_RESULT_SUCCESS == result) {
        result = (*m_output_mix_environmental_reverb_)->SetEnvironmentalReverbProperties(m_output_mix_environmental_reverb_, &m_reverbsettings_);//设置某种指定的混响音效，比如走廊混响
        (void) result;
    }

    //3.创建播放器
    SLDataLocator_AndroidSimpleBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,2};//指定了两个buffer队列
    SLDataFormat_PCM pcm = {//指定设备进行播放的pcm格式参数，按照指定的参数设置进行播放
            SL_DATAFORMAT_PCM,
            2,
            _GetCurrentSampleRateForOpenSLES(m_sample_rate),
            SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
            SL_BYTEORDER_LITTLEENDIAN
    };

    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, m_output_mix_object_};
    SLDataSource slDataSource = {&android_queue, &pcm};
    SLDataSink audioSink = {&outputMix, NULL};

    //SL_IID_MUTESOLO:声道切换控制 SL_IID_VOLUME:音量控制 SL_IID_PLAYBACKRATE:
    const SLInterfaceID ids[4] = {SL_IID_BUFFERQUEUE, SL_IID_MUTESOLO, SL_IID_VOLUME, SL_IID_PLAYBACKRATE};//指定使能缓存队列和音量操作的接口
    const SLboolean req[4] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};

    //根据引擎创建音频播放器实例
    result = (*m_engine_engine_)->CreateAudioPlayer(m_engine_engine_, &m_pcm_player_object_, &slDataSource, &audioSink, 4, ids, req);

    // 初始化播放器
    (*m_pcm_player_object_)->Realize(m_pcm_player_object_, SL_BOOLEAN_FALSE);
    //得到接口后调用  获取Player接口
    (*m_pcm_player_object_)->GetInterface(m_pcm_player_object_, SL_IID_PLAY, &m_pcm_player_play_);//根据音频播放器实例获取到音频播放接口

    //4.设置缓存队列和回调函数
    (*m_pcm_player_object_)->GetInterface(m_pcm_player_object_, SL_IID_BUFFERQUEUE, &pcmBufferQueue);//根据音频播放器实例获取到音频缓存队列的接口
    (*pcmBufferQueue)->RegisterCallback(pcmBufferQueue, _PcmBufferCallBack, this);

    //获取音量接口
    (*m_pcm_player_object_)->GetInterface(m_pcm_player_object_, SL_IID_VOLUME, &m_pcm_player_volume_);
    SetVolume(m_volume_percent_);

    SetChannelType(m_channel_type_);

    //获取声道接口
    (*m_pcm_player_object_)->GetInterface(m_pcm_player_object_, SL_IID_MUTESOLO, &m_pcm_player_mute_);

    //5.设置播放状态
    (*m_pcm_player_play_)->SetPlayState(m_pcm_player_play_, SL_PLAYSTATE_PLAYING);

    //6.调用一次pcm播放的回调函数，表示启动opensles播放
    _PcmBufferCallBack(pcmBufferQueue, this);
    LOGI("initOpenSLES is end");
}

void *_DecodePlay(void *data) {
    WLAudio *wlAudio = (WLAudio *) data;
    wlAudio->InitOpenSLES();
//    pthread_exit(&wlAudio->thread_play);
    return 0;
}

//线程函数：用于获取缓冲区中的播放的pcm数据上报
void *_PcmCallBack(void *data) {
    WLAudio *wlAudio = (WLAudio *) data;
    wlAudio->m_buffer_queue = new WLBufferQueue(wlAudio->m_play_status);
    while ((wlAudio->m_play_status != NULL) && !wlAudio->m_play_status->m_is_exit) {
        WLPcmBean *pcmBean = NULL;
        wlAudio->m_buffer_queue->GetBuffer(&pcmBean);
        if (pcmBean == NULL) {
            continue;
        }
        if (pcmBean->m_buffsize <= wlAudio->m_default_pcm_size) {//不用分包
            if (wlAudio->m_is_record_pcm) {
                wlAudio->m_call_java->OnCallPcmToAAC(CHILD_THREAD, pcmBean->m_buffer, pcmBean->m_buffsize);
            }
            if (wlAudio->showPcm) {
                wlAudio->m_call_java->OnCallPcmData(CHILD_THREAD, pcmBean->m_buffer, pcmBean->m_buffsize);
            }
        } else {//分包上报
            int pack_num = pcmBean->m_buffsize / wlAudio->m_default_pcm_size;
            int pack_sub = pcmBean->m_buffsize % wlAudio->m_default_pcm_size;//剩余的size
            for (int i = 0; i < pack_num; ++i) {
                char *bf = (char *) malloc(wlAudio->m_default_pcm_size);
                memcpy(bf, pcmBean->m_buffer + i * wlAudio->m_default_pcm_size, wlAudio->m_default_pcm_size);
                if (wlAudio->m_is_record_pcm) {
                    wlAudio->m_call_java->OnCallPcmToAAC(CHILD_THREAD, bf, wlAudio->m_default_pcm_size);
                }
                if (wlAudio->showPcm) {
                    wlAudio->m_call_java->OnCallPcmData(CHILD_THREAD, bf, wlAudio->m_default_pcm_size);
                }
                free(bf);
                bf = NULL;
            }

            if (pack_sub > 0) {
                char *bf = (char *) malloc(pack_sub);
                memcpy(bf, pcmBean->m_buffer + pack_num * wlAudio->m_default_pcm_size, pack_sub);
                if (wlAudio->m_is_record_pcm) {
                    wlAudio->m_call_java->OnCallPcmToAAC(CHILD_THREAD, bf, pack_sub);
                }
                if (wlAudio->showPcm) {
                    wlAudio->m_call_java->OnCallPcmData(CHILD_THREAD, bf, pack_sub);
                }
                free(bf);
                bf = NULL;
            }
        }
        delete pcmBean;
        pcmBean = NULL;
    }
//    pthread_exit(&wlAudio->pcmCallBackThread);
    return 0;
}

void WLAudio::Play() {
    if ((m_play_status != NULL) && !m_play_status->m_is_exit) {
        pthread_create(&m_play_thread_, NULL, _DecodePlay, this);//创建音频播放子线程，初始化opensles相关流程，并注册好播放回调
        pthread_create(&m_pcm_callback_thread_, NULL, _PcmCallBack, this);//创建pcm数据的回调子线程
    }
}

//音频暂停播放Opensl
void WLAudio::Pause() {
    if (m_pcm_player_play_ != NULL) {
        (*m_pcm_player_play_)->SetPlayState(m_pcm_player_play_, SL_PLAYSTATE_PAUSED);
    }
}

//音频恢复播放Opensl
void WLAudio::Resume() {
    if (m_pcm_player_play_ != NULL) {
        (*m_pcm_player_play_)->SetPlayState(m_pcm_player_play_, SL_PLAYSTATE_PLAYING);
    }
}

//音频停止播放Opensl
void WLAudio::Stop() {
    if (m_pcm_player_play_ != NULL) {
        (*m_pcm_player_play_)->SetPlayState(m_pcm_player_play_, SL_PLAYSTATE_STOPPED);
    }
}

void WLAudio::Release() {
    Stop();

    if (m_buffer_queue != NULL) {
        m_buffer_queue->NoticeThread();
        pthread_join(m_pcm_callback_thread_, NULL);
        delete m_buffer_queue;
        m_buffer_queue = NULL;
    }

    if (m_queue != NULL) {
        delete m_queue;
        m_queue = NULL;
        pthread_join(m_play_thread_, NULL);
    }

    if (m_pcm_player_object_ != NULL) {
        (*m_pcm_player_object_)->Destroy(m_pcm_player_object_);
        m_pcm_player_object_ = NULL;
        m_pcm_player_play_ = NULL;
        pcmBufferQueue = NULL;
        m_pcm_player_volume_ = NULL;
        m_pcm_player_mute_ = NULL;
    }

    if (m_output_mix_object_ != NULL) {
        (*m_output_mix_object_)->Destroy(m_output_mix_object_);
        m_output_mix_object_ = NULL;
        m_output_mix_environmental_reverb_ = NULL;
    }

    if (m_engine_object_ != NULL) {
        (*m_engine_object_)->Destroy(m_engine_object_);
        m_engine_object_ = NULL;
        m_engine_engine_ = NULL;
    }

    if (m_buffer_ != NULL) {
        free(m_buffer_);
        m_buffer_ = NULL;
    }

    if (m_out_buffer_ != NULL) {
        m_out_buffer_ = NULL;
    }

    if (m_soundtouch_ != NULL) {
        delete m_soundtouch_;
        m_soundtouch_ = NULL;
    }

    if (sampleBuffer != NULL) {
        free(sampleBuffer);
        sampleBuffer = NULL;
    }

    if (m_avcodec_ctx != NULL) {
        avcodec_close(m_avcodec_ctx);
        avcodec_free_context(&m_avcodec_ctx);
        m_avcodec_ctx = NULL;//虽然 avcodec_free_context 已经做了这件事，但是显式赋值可以使代码更加清晰和健壮
    }

    if (m_play_status != NULL) {
        m_play_status = NULL;
    }

    if (m_call_java != NULL) {
        m_call_java = NULL;
    }
}

//控制音频音量调整（opensl）
void WLAudio::SetVolume(int percent) {
    m_volume_percent_ = percent;
    if (m_pcm_player_volume_ != NULL) {
        //下面是为了让音量调节平滑一点
        if (percent > 30) {
            (*m_pcm_player_volume_)->SetVolumeLevel(m_pcm_player_volume_, (100 - percent) * (-20));//percent:100 原声 percent:0 静音
        } else if (percent > 25) {
            (*m_pcm_player_volume_)->SetVolumeLevel(m_pcm_player_volume_, (100 - percent) * (-22));//percent:100 原声 percent:0 静音
        } else if (percent > 20) {
            (*m_pcm_player_volume_)->SetVolumeLevel(m_pcm_player_volume_, (100 - percent) * (-25));//percent:100 原声 percent:0 静音
        } else if (percent > 15) {
            (*m_pcm_player_volume_)->SetVolumeLevel(m_pcm_player_volume_, (100 - percent) * (-28));//percent:100 原声 percent:0 静音
        } else if (percent > 10) {
            (*m_pcm_player_volume_)->SetVolumeLevel(m_pcm_player_volume_, (100 - percent) * (-30));//percent:100 原声 percent:0 静音
        } else if (percent > 5) {
            (*m_pcm_player_volume_)->SetVolumeLevel(m_pcm_player_volume_, (100 - percent) * (-34));//percent:100 原声 percent:0 静音
        } else if (percent > 3) {
            (*m_pcm_player_volume_)->SetVolumeLevel(m_pcm_player_volume_, (100 - percent) * (-37));//percent:100 原声 percent:0 静音
        } else if (percent > 0) {
            (*m_pcm_player_volume_)->SetVolumeLevel(m_pcm_player_volume_, (100 - percent) * (-40));//percent:100 原声 percent:0 静音
        } else {
            (*m_pcm_player_volume_)->SetVolumeLevel(m_pcm_player_volume_, (100 - percent) * (-100));//percent:100 原声 percent:0 静音
        }
    }
}

//控制音频左右声道的播放（opensl）
void WLAudio::SetChannelType(int channel_type) {
    m_channel_type_ = channel_type;
    if (m_pcm_player_mute_ != NULL) {
        if (channel_type == 0) {//rigt 右声道
            (*m_pcm_player_mute_)->SetChannelMute(m_pcm_player_mute_, 1, false);//0右声道  1左声道  第三个参数表示是否关闭
            (*m_pcm_player_mute_)->SetChannelMute(m_pcm_player_mute_, 0, true);
        } else if (channel_type == 1) {//left 左声道
            (*m_pcm_player_mute_)->SetChannelMute(m_pcm_player_mute_, 1, true);
            (*m_pcm_player_mute_)->SetChannelMute(m_pcm_player_mute_, 0, false);
        } else if (channel_type == 2) {//center 立体声
            (*m_pcm_player_mute_)->SetChannelMute(m_pcm_player_mute_, 1, false);
            (*m_pcm_player_mute_)->SetChannelMute(m_pcm_player_mute_, 0, false);
        }
    }
}

//设置音调值(soundtouch)
void WLAudio::SetPitch(float pitch) {
    this->pitch = pitch;
    LOGI("WLAudio setPitch: %f soundTouch: %p", this->pitch, m_soundtouch_);
    if (m_soundtouch_ != NULL) {
        m_soundtouch_->setPitch(pitch);
    }
}

//设置音频播放速度值(soundtouch)
void WLAudio::SetSpeed(float speed) {
    this->speed = speed;
    LOGI("WLAudio setSpeed: %f soundTouch: %p", this->speed, m_soundtouch_);
    if (m_soundtouch_ != NULL) {
        m_soundtouch_->setTempo(speed);
    }
}

//根据音频包的PCM数据计算得到音量的分贝值
int WLAudio::GetPCMDB(char *pcmdata, size_t pcmsize) {
    int db = 0;
    short int pervalue = 0;
    double sum = 0;
    for (int i = 0; i < pcmsize; i += 2) {
        memcpy(&pervalue, (pcmdata + i), 2);//转为short
        sum += abs(pervalue);//short值累加
    }

    sum = sum / (pcmsize / 2);//求平均
    if (sum > 0) {
        db = (int) 20.0 * log10(sum);//计算得到分贝
    }
    return db;
}

int WLAudio::GetSoundTouchData() {
    while ((m_play_status != NULL) && !m_play_status->m_is_exit) {
        m_out_buffer_ = NULL;
        if (m_finished_) {
            m_finished_ = false;
            m_data_size_ = _ResampleAudio(reinterpret_cast<void **>(&m_out_buffer_));//返回当前解码并重采样后的pcm的字节数
            if (m_data_size_ > 0) {
                for (int i = 0; i < m_data_size_ / 2 + 1; ++i) {
                    sampleBuffer[i] = (m_out_buffer_[i * 2] | ((m_out_buffer_[i * 2 + 1]) << 8));//将两个字节组成一个short
                }
//                sampleBuffer = (SAMPLETYPE *) out_buffer;
//                LOGI("nb is %d", nb);
                m_soundtouch_->putSamples(sampleBuffer, m_nb_);
                m_num_ = m_soundtouch_->receiveSamples(sampleBuffer, m_data_size_ / 4);//返回的是单个音频包单通道的pcm采样点数
            } else {//没有输入的pcm数据，刷新变速变调实例的缓冲
                m_soundtouch_->flush();
            }
        }

        if (m_num_ == 0) {//没有接收到变速变调返回的数据，继续进行获取操作
            m_finished_ = true;
            continue;
        } else {
            if (m_out_buffer_ == NULL) {//如果获取采样后的数据为空，但是收到了变速变调的数据，则继续获取变速变调的数据，读完为止
                m_num_ = m_soundtouch_->receiveSamples(sampleBuffer, m_data_size_ / 4);
                if (m_num_ == 0) {
                    m_finished_ = true;
                    continue;
                }
            }
            return m_num_;
        }
    }
    return 0;
}

void WLAudio::StartStopRecord(bool flag) {
    m_is_record_pcm = flag;
}

//解码并重采样，得到一个音频包packet的解码pcm数据,一个音频包可能可以解码出来多个Frame
int WLAudio::_ResampleAudio(void **pcmbuf) {
    m_data_size_ = 0;
    while ((m_play_status != NULL) && !m_play_status->m_is_exit) {
        if (m_play_status->m_seek) {//seek状态，不进行下面的解码
            av_usleep(100 * 1000);//100毫秒
            continue;
        }

        if (m_queue->GetQueueSize() == 0) {//加载中
            if (!m_play_status->m_load) {
                m_play_status->m_load = true;
                m_call_java->OnCallLoad(CHILD_THREAD, true);
            }
            av_usleep(100 * 1000);//100毫秒
            continue;
        } else {//缓冲区中有数据，结束加载
            if (m_play_status->m_load) {
                m_play_status->m_load = false;
                m_call_java->OnCallLoad(CHILD_THREAD, false);
            }
        }

        int ret = -1;
        if (readFrameFinish) {
            m_avpacket_ = av_packet_alloc();
            m_queue->GetAVPacket(m_avpacket_);

            pthread_mutex_lock(&m_codec_mutex);
            ret = avcodec_send_packet(m_avcodec_ctx, m_avpacket_);
            if (ret != 0) {
                av_packet_free(&m_avpacket_);
                pthread_mutex_unlock(&m_codec_mutex);
                continue;
            }
        }

        m_avframe_ = av_frame_alloc();
        ret = avcodec_receive_frame(m_avcodec_ctx, m_avframe_);
        if (ret == 0) {//解码成功，获取解码数据，下次会继续进行获取，取完解码数据为止
            readFrameFinish = false;
            if ((m_avframe_->channels > 0) && (m_avframe_->channel_layout == 0)) {
                m_avframe_->channel_layout = av_get_default_channel_layout(m_avframe_->channels);
            } else if ((m_avframe_->channels == 0) && (m_avframe_->channel_layout > 0)) {
                m_avframe_->channels = av_get_channel_layout_nb_channels(m_avframe_->channel_layout);
            }

            //2.音频重采样
            SwrContext *swr_ctx;
            swr_ctx = swr_alloc_set_opts(
                    NULL,
                    AV_CH_LAYOUT_STEREO,//输出
                    AV_SAMPLE_FMT_S16,
                    m_avframe_->sample_rate,
                    m_avframe_->channel_layout,//输入
                    (AVSampleFormat) (m_avframe_->format),
                    m_avframe_->sample_rate,
                    NULL, NULL);
            if (!swr_ctx || (swr_init(swr_ctx) < 0)) {
                av_packet_free(&m_avpacket_);
                av_free(m_avpacket_);
                m_avpacket_ = NULL;
                av_frame_free(&m_avframe_);
                av_free(m_avframe_);
                m_avframe_ = NULL;
                swr_free(&swr_ctx);
                readFrameFinish = true;
                pthread_mutex_unlock(&m_codec_mutex);
                continue;
            }

            //重采样，返回单个包的单通道采样点数:对于AAC，一般是1024个采样点
            m_nb_ = swr_convert(swr_ctx, &m_buffer_, m_avframe_->nb_samples, (const uint8_t **) (m_avframe_->data), m_avframe_->nb_samples);
            int out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
            m_data_size_ = m_nb_ * out_channels * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);//计算得到一个音频包的pcm字节数

            now_time = m_avframe_->pts * av_q2d(time_base);//计算得到当前音频包的解码时间戳
            if (now_time < clock) {//保证clock递增
                now_time = clock;
            }
            //clock = now_time;
            //LOGI("resampleAudio audio clock: %lf", clock);
            *pcmbuf = m_buffer_;

//            fwrite(buffer, data_size, 1, outFile);
            if (LOG_DEBUG) {
//                LOGI("data size %d", data_size);
            }

//            av_packet_free(&avPacket);
//            av_free(avPacket);
//            avPacket = NULL;

            av_frame_free(&m_avframe_);
            av_free(m_avframe_);
            m_avframe_ = NULL;
            swr_free(&swr_ctx);
            swr_ctx = NULL;
            pthread_mutex_unlock(&m_codec_mutex);
            break;
        } else {//没有获取到解码数据，继续往解码器塞数据解码
            readFrameFinish = true;

            av_packet_free(&m_avpacket_);
            av_free(m_avpacket_);
            m_avpacket_ = NULL;
            av_frame_free(&m_avframe_);
            av_free(m_avframe_);
            m_avframe_ = NULL;

            pthread_mutex_unlock(&m_codec_mutex);
            continue;
        }
    }
//    fclose(outFile);
//    LOGI("fclose the pcm file");
    return m_data_size_;
}

SLuint32 WLAudio::_GetCurrentSampleRateForOpenSLES(int sample_rate) {
    SLuint32 rate = 0;
    switch (sample_rate) {
        case 8000:
            rate = SL_SAMPLINGRATE_8;
            break;
        case 11025:
            rate = SL_SAMPLINGRATE_11_025;
            break;
        case 12000:
            rate = SL_SAMPLINGRATE_12;
            break;
        case 16000:
            rate = SL_SAMPLINGRATE_16;
            break;
        case 22050:
            rate = SL_SAMPLINGRATE_22_05;
            break;
        case 24000:
            rate = SL_SAMPLINGRATE_24;
            break;
        case 32000:
            rate = SL_SAMPLINGRATE_32;
            break;
        case 44100:
            rate = SL_SAMPLINGRATE_44_1;
            break;
        case 48000:
            rate = SL_SAMPLINGRATE_48;
            break;
        case 64000:
            rate = SL_SAMPLINGRATE_64;
            break;
        case 88200:
            rate = SL_SAMPLINGRATE_88_2;
            break;
        case 96000:
            rate = SL_SAMPLINGRATE_96;
            break;
        case 192000:
            rate = SL_SAMPLINGRATE_192;
            break;
        default:
            rate = SL_SAMPLINGRATE_44_1;
            break;
    }
    return rate;
}
