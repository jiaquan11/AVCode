#include "wl_audio.h"

WLAudio::WLAudio(int sample_rate, WLPlayStatus *play_status, CallJava *call_java) {
    m_play_status = play_status;
    m_sample_rate = sample_rate;
    m_call_java = call_java;
    m_is_cut = false;
    m_end_time = 0;
    m_show_pcm = false;

    m_packet_queue = new WLQueue(m_play_status);
    m_convert_buffer_ = (uint8_t *) (av_malloc(sample_rate * 2 * 2));//分配了一秒的音频pcm数据内存
    memset(m_convert_buffer_, 0, sample_rate * 2 * 2);
    m_avpacket_ = av_packet_alloc();
    m_avframe_ = av_frame_alloc();
    /**
     * 音频变速变调功能初始化
     */
    m_sample_buffer = static_cast<SAMPLETYPE *>(malloc(sample_rate * 2 * 2));//分配了一秒的音频pcm数据内存
    memset(m_sample_buffer, 0, sample_rate * 2 * 2);
    m_soundtouch_ = new SoundTouch();
    m_soundtouch_->setSampleRate(sample_rate);
    m_soundtouch_->setChannels(2);
    m_soundtouch_->setPitch(m_pitch_);
    m_soundtouch_->setTempo(m_speed_);
    LOGI("WLAudio construct pitch: %f speed: %f", m_pitch_, m_speed_);
    pthread_mutex_init(&m_codec_mutex, NULL);
}

WLAudio::~WLAudio() {
    pthread_mutex_destroy(&m_codec_mutex);
}

/**
 * OpenSLES注册的回调函数，会由OpenSLES主动调用，
 * 直接将pcm数据放入OpenSLES中的缓冲队列中进行播放
 */
void _PcmBufferPlayCallBack(SLAndroidSimpleBufferQueueItf bf, void *arg) {
    WLAudio *audio = (WLAudio *) (arg);
    if (audio != NULL) {
        /**
         * 返回的是当前音频包解码后的PCM的采样点数(单个)
         * 这里可能输出采样点数是不均匀的，因为有缓冲区的存在，所以可能会有多个音频包的pcm数据一起输出
         */
        int sample_count = audio->GetSoundTouchData();//返回的是当前音频包解码后的PCM的采样点数(单通道音频采样点数)
        if (sample_count > 0) {
            audio->m_clock += (sample_count * 2 * 2) / ((double) (audio->m_sample_rate * 2 * 2));//累加一下播放一段pcm所耗费的时间
            if (audio->m_clock - audio->m_last_time >= 0.1) {//100毫秒上报一次当前的音频播放时间戳
                audio->m_last_time = audio->m_clock;
                audio->m_call_java->OnCallTimeInfo(CHILD_THREAD, audio->m_clock, audio->m_duration);
            }

            audio->m_buffer_queue->PutBuffer((char*)audio->m_sample_buffer, sample_count * 4);//将解码的Pcm数据放入缓冲区，用于另外一个线程获取用于上报
            audio->m_call_java->OnCallVolumeDB(CHILD_THREAD, audio->GetPCMDB(reinterpret_cast<char *>(audio->m_sample_buffer), sample_count * 2 * 2));//上报音频的分贝值

            (*audio->m_pcm_buffer_queue)->Enqueue(audio->m_pcm_buffer_queue, audio->m_sample_buffer, sample_count * 2 * 2);//将pcm数据丢入opensles队列进行播放
            if (audio->m_is_cut) {
                if (audio->m_clock >= audio->m_end_time) {
                    audio->m_play_status->m_is_exit = true;
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
        m_reverbsettings_ = SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;//走廊混响
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
    //SL_IID_BUFFERQUEUE：缓冲队列 SL_IID_MUTESOLO:声道切换控制 SL_IID_VOLUME:音量控制 SL_IID_PLAYBACKRATE：播放速度控制
    const SLInterfaceID ids[4] = {SL_IID_BUFFERQUEUE, SL_IID_MUTESOLO, SL_IID_VOLUME, SL_IID_PLAYBACKRATE};//指定使能缓存队列和音量操作的接口
    const SLboolean req[4] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};

    //根据引擎创建音频播放器实例
    result = (*m_engine_engine_)->CreateAudioPlayer(m_engine_engine_, &m_pcm_player_object_, &slDataSource, &audioSink, 4, ids, req);
    //播放器实例化
    (*m_pcm_player_object_)->Realize(m_pcm_player_object_, SL_BOOLEAN_FALSE);
    //得到接口后调用  获取Player接口
    (*m_pcm_player_object_)->GetInterface(m_pcm_player_object_, SL_IID_PLAY, &m_pcm_player_play_);//根据音频播放器实例获取到音频播放接口

    //4.设置缓存队列和回调函数
    (*m_pcm_player_object_)->GetInterface(m_pcm_player_object_, SL_IID_BUFFERQUEUE, &m_pcm_buffer_queue);//根据音频播放器实例获取到音频缓存队列的接口
    (*m_pcm_buffer_queue)->RegisterCallback(m_pcm_buffer_queue, _PcmBufferPlayCallBack, this);

    //获取音量接口
    (*m_pcm_player_object_)->GetInterface(m_pcm_player_object_, SL_IID_VOLUME, &m_pcm_player_volume_);
    SetVolume(m_volume_percent_);

    //获取声道接口
    (*m_pcm_player_object_)->GetInterface(m_pcm_player_object_, SL_IID_MUTESOLO, &m_pcm_player_channel_);
    SetChannelType(m_channel_type_);

    //5.设置播放状态
    (*m_pcm_player_play_)->SetPlayState(m_pcm_player_play_, SL_PLAYSTATE_PLAYING);

    //6.调用一次pcm播放的回调函数，表示启动opensles播放
    _PcmBufferPlayCallBack(m_pcm_buffer_queue, this);//主动触发,启动opensles播放
    LOGI("initOpenSLES is end");
}

/**
 * 播放线程函数：用于初始化opensles相关流程，并注册好播放回调
 */
void *_PlayAudio(void *arg) {
    WLAudio *audio = (WLAudio *) arg;
    audio->InitOpenSLES();
//    pthread_exit(&wlAudio->thread_play);
    return 0;
}

/**
 * 线程函数：用于获取缓冲区中的播放的pcm数据上报
 */
void *_PcmBufferReportCallBack(void *arg) {
    WLAudio *audio = (WLAudio *) arg;
    audio->m_buffer_queue = new WLBufferQueue(audio->m_play_status);
    while ((audio->m_play_status != NULL) && !audio->m_play_status->m_is_exit) {
        WLPcmBean *pcm_bean = NULL;
        audio->m_buffer_queue->GetBuffer(&pcm_bean);
        if (pcm_bean == NULL) {
            LOGE("WLAudio _PcmBufferReportCallBack pcm_bean is NULL");
            av_usleep(2 * 1000);//休眠2毫秒
            continue;
        }
        if (pcm_bean->m_buffsize <= audio->kDefaultPcmSize) {//不用分包
            if (audio->m_is_record_pcm) {
                audio->m_call_java->OnCallPcmToAAC(CHILD_THREAD, pcm_bean->m_buffer, pcm_bean->m_buffsize);
            }
            if (audio->m_show_pcm) {
                audio->m_call_java->OnCallPcmData(CHILD_THREAD, pcm_bean->m_buffer, pcm_bean->m_buffsize);
            }
        } else {//分包上报
            int pack_num = pcm_bean->m_buffsize / audio->kDefaultPcmSize;
            int pack_sub = pcm_bean->m_buffsize % audio->kDefaultPcmSize;//剩余的size
            for (int i = 0; i < pack_num; ++i) {
                char *bf = (char *) malloc(audio->kDefaultPcmSize);
                memcpy(bf, pcm_bean->m_buffer + i * audio->kDefaultPcmSize, audio->kDefaultPcmSize);
                if (audio->m_is_record_pcm) {
                    audio->m_call_java->OnCallPcmToAAC(CHILD_THREAD, bf, audio->kDefaultPcmSize);
                }
                if (audio->m_show_pcm) {
                    audio->m_call_java->OnCallPcmData(CHILD_THREAD, bf, audio->kDefaultPcmSize);
                }
                free(bf);
                bf = NULL;
            }

            if (pack_sub > 0) {
                char *bf = (char *) malloc(pack_sub);
                memcpy(bf, pcm_bean->m_buffer + pack_num * audio->kDefaultPcmSize, pack_sub);
                if (audio->m_is_record_pcm) {
                    audio->m_call_java->OnCallPcmToAAC(CHILD_THREAD, bf, pack_sub);
                }
                if (audio->m_show_pcm) {
                    audio->m_call_java->OnCallPcmData(CHILD_THREAD, bf, pack_sub);
                }
                free(bf);
                bf = NULL;
            }
        }
        delete pcm_bean;
        pcm_bean = NULL;
    }
//    pthread_exit(&wlAudio->pcmCallBackThread);
    return 0;
}

void WLAudio::Play() {
    if ((m_play_status != NULL) && !m_play_status->m_is_exit) {
        int ret = pthread_create(&m_play_thread_, NULL, _PlayAudio, this);//创建音频播放子线程，初始化opensles相关流程，并注册好播放回调
        if (ret != 0) {
            LOGE("Create Play Audio Thread failed!");
        }
        ret = pthread_create(&m_pcm_report_callback_thread_, NULL, _PcmBufferReportCallBack, this);//创建pcm数据上报的子线程
        if (ret != 0) {
            LOGE("Create Pcm Call Back Thread failed!");
        }
    }
}

/**
 * 暂停音频opensl播放
 */
void WLAudio::Pause() {
    if (m_pcm_player_play_ != NULL) {
        (*m_pcm_player_play_)->SetPlayState(m_pcm_player_play_, SL_PLAYSTATE_PAUSED);
    }
}

/**
 * 恢复音频opensl播放
 */
void WLAudio::Resume() {
    if (m_pcm_player_play_ != NULL) {
        (*m_pcm_player_play_)->SetPlayState(m_pcm_player_play_, SL_PLAYSTATE_PLAYING);
    }
}

/**
 * 停止音频opensl播放
 */
void WLAudio::Stop() {
    if (m_pcm_player_play_ != NULL) {
        (*m_pcm_player_play_)->SetPlayState(m_pcm_player_play_, SL_PLAYSTATE_STOPPED);
    }
}

void WLAudio::Release() {
    Stop();//停止opensl音频播放
    if (m_packet_queue != NULL) {
        m_packet_queue->NoticeQueue();
    }
    if (m_buffer_queue != NULL) {
        m_buffer_queue->NoticeQueue();
    }
    void *thread_ret;
    int result =  pthread_join(m_pcm_report_callback_thread_, &thread_ret);
    LOGI("audio m_pcm_report_callback_thread_ join result: %d", result);
    if (result != 0) {
        switch (result) {
            case ESRCH:
                LOGE("audio m_pcm_report_callback_thread_ pthread_join failed: Thread not found (ESRCH)");
                break;
            case EINVAL:
                LOGE("audio m_pcm_report_callback_thread_ pthread_join failed: Invalid thread or thread already detached (EINVAL)");
                break;
            case EDEADLK:
                LOGE("audio m_pcm_report_callback_thread_ pthread_join failed: Deadlock detected (EDEADLK)");
                break;
            default:
                LOGE("audio m_pcm_report_callback_thread_ pthread_join failed: Unknown error (%d)", result);
        }
        // Exit or perform additional actions if needed
        exit(EXIT_FAILURE);
    } else {
        LOGI("audio m_pcm_report_callback_thread_ Thread returned: %ld", (long)thread_ret);
    }

    result = pthread_join(m_play_thread_, &thread_ret);
    LOGI("audio m_play_thread_ join result: %d", result);
    if (result != 0) {
        switch (result) {
            case ESRCH:
                LOGE("audio m_play_thread_ pthread_join failed: Thread not found (ESRCH)");
                break;
            case EINVAL:
                LOGE("audio m_play_thread_ pthread_join failed: Invalid thread or thread already detached (EINVAL)");
                break;
            case EDEADLK:
                LOGE("audio m_play_thread_ pthread_join failed: Deadlock detected (EDEADLK)");
                break;
            default:
                LOGE("audio m_play_thread_ pthread_join failed: Unknown error (%d)", result);
        }
        // Exit or perform additional actions if needed
        exit(EXIT_FAILURE);
    } else {
        LOGI("audio m_play_thread_ Thread returned: %ld", (long)thread_ret);
    }

    if (m_buffer_queue != NULL) {
        delete m_buffer_queue;
        m_buffer_queue = NULL;
    }
    if (m_packet_queue != NULL) {
        delete m_packet_queue;
        m_packet_queue = NULL;
    }

    if (m_pcm_player_object_ != NULL) {
        (*m_pcm_player_object_)->Destroy(m_pcm_player_object_);
        m_pcm_player_object_ = NULL;
        m_pcm_player_play_ = NULL;
        m_pcm_buffer_queue = NULL;
        m_pcm_player_volume_ = NULL;
        m_pcm_player_channel_ = NULL;
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
    if (m_convert_buffer_ != NULL) {
        free(m_convert_buffer_);
        m_convert_buffer_ = NULL;
    }
    if (m_soundtouch_ != NULL) {
        delete m_soundtouch_;
        m_soundtouch_ = NULL;
    }
    if (m_sample_buffer != NULL) {
        free(m_sample_buffer);
        m_sample_buffer = NULL;
    }
    if (m_avcodec_ctx != NULL) {
        pthread_mutex_lock(&m_codec_mutex);
        avcodec_free_context(&m_avcodec_ctx);
        m_avcodec_ctx = NULL;//虽然 avcodec_free_context 已经做了这件事，但是显式赋值可以使代码更加清晰和健壮
        pthread_mutex_unlock(&m_codec_mutex);
    }
    if (m_avpacket_ != NULL) {
        av_packet_free(&m_avpacket_);
        m_avpacket_ = NULL;
    }
    if (m_avframe_ != NULL) {
        av_frame_free(&m_avframe_);
        m_avframe_ = NULL;
    }
    if (m_swr_ctx_ != NULL) {
        swr_free(&m_swr_ctx_);
        m_swr_ctx_ = NULL;
    }
    if (m_play_status != NULL) {
        m_play_status = NULL;
    }
    if (m_call_java != NULL) {
        m_call_java = NULL;
    }
}

/**
 * 设置音频播放音量
 * 通过不同的音量百分比范围内使用不同的衰减系数，使得音量调节更为平滑
 * SetVolumeLevel的第二个参数是衰减系数，绝对值越大，衰减越快，音量越小
 * 随着音量百分比的减小，逐渐增加音量的衰减系数，从而实现更大的衰减，使音量变得更小
 */
void WLAudio::SetVolume(int percent) {
    m_volume_percent_ = percent;
    if (m_pcm_player_volume_ != NULL) {
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

/**
 * 设置音频播放声道
 * @param channel_type
 */
void WLAudio::SetChannelType(int channel_type) {
    m_channel_type_ = channel_type;
    if (m_pcm_player_channel_ != NULL) {
        if (channel_type == 0) {//right 右声道
            (*m_pcm_player_channel_)->SetChannelMute(m_pcm_player_channel_, 1, false);//0右声道  1左声道  第三个参数表示是否关闭
            (*m_pcm_player_channel_)->SetChannelMute(m_pcm_player_channel_, 0, true);
        } else if (channel_type == 1) {//left 左声道
            (*m_pcm_player_channel_)->SetChannelMute(m_pcm_player_channel_, 1, true);
            (*m_pcm_player_channel_)->SetChannelMute(m_pcm_player_channel_, 0, false);
        } else if (channel_type == 2) {//center 立体声
            (*m_pcm_player_channel_)->SetChannelMute(m_pcm_player_channel_, 1, false);
            (*m_pcm_player_channel_)->SetChannelMute(m_pcm_player_channel_, 0, false);
        }
    }
}

/**
 * 设置音频播放音调
 * @param pitch
 */
void WLAudio::SetPitch(float pitch) {
    m_pitch_ = pitch;
    LOGI("WLAudio SetPitch: %f", m_pitch_);
    if (m_soundtouch_ != NULL) {
        m_soundtouch_->setPitch(pitch);
    }
}

/**
 * 设置音频播放速度
 * @param speed
 */
void WLAudio::SetSpeed(float speed) {
    m_speed_ = speed;
    LOGI("WLAudio SetSpeed: %f", m_speed_);
    if (m_soundtouch_ != NULL) {
        m_soundtouch_->setTempo(speed);
    }
}

/**
 * 根据音频包的PCM数据计算得到音量的分贝值
 * @param pcm_data
 * @param pcm_size
 */
int WLAudio::GetPCMDB(char *pcm_data, size_t pcm_size) {
    int db = 0;
    short int pervalue = 0;
    double sum = 0;
    for (int i = 0; i < pcm_size; i += 2) {
        memcpy(&pervalue, (pcm_data + i), 2);//转为short
        sum += abs(pervalue);//short值累加
    }
    sum = sum / (pcm_size / 2);//求平均
    if (sum > 0) {
        db = (int) 20.0 * log10(sum);//计算得到分贝
    }
    return db;
}

/**
 * 获取变速变调后的音频数据
 * 必须得获取soundtouch处理后返回的有效数据，否则会一直获取
 */
int WLAudio::GetSoundTouchData() {
    int receive_samples = 0;
    while ((m_play_status != NULL) && !m_play_status->m_is_exit) {
        uint8_t *out_buffer = NULL;
        if (m_need_receive_samples_) {
            m_need_receive_samples_ = false;
            m_data_size_ = _ResampleAudio(reinterpret_cast<void **>(&out_buffer));//返回当前解码并重采样后的pcm的字节数
            if (m_data_size_ > 0) {
                for (int i = 0; i < m_data_size_ / 2; ++i) {
                    m_sample_buffer[i] = (out_buffer[i * 2] | ((out_buffer[i * 2 + 1]) << 8));//将两个字节组成一个short
                }
                if (m_data_size_ % 2 != 0) {
                    m_sample_buffer[m_data_size_ / 2] = out_buffer[m_data_size_ - 1];//处理剩余单一字节
                }

                /**
                 * 这里要注意下：soundtouch的putSamples和receiveSamples第一个参数双通道的pcm数据，而不是单通道的pcm数据，
                 * 但是需要转换为short类型的数据，而不是char类型的数据。
                 * 第二个参数必须是单通道的pcm采样点数(最大采样点数)
                 * 参数要传对，否则会有声音卡顿等播放问题
                 */
                m_soundtouch_->putSamples(m_sample_buffer, m_nb_);//将解码得到的pcm数据放入变速变调实例处理
                receive_samples = m_soundtouch_->receiveSamples(m_sample_buffer, m_nb_);//返回的是单个音频包单通道的pcm采样点数
            } else {//没有输入的pcm数据，刷新变速变调实例的缓冲
                m_soundtouch_->flush();
            }
        }

        if (receive_samples == 0) {//没有接收到变速变调返回的数据，继续进行获取操作
            m_need_receive_samples_ = true;
            continue;
        } else {
            if (out_buffer == NULL) {//多次获取变速变调的数据，读完为止
                receive_samples = m_soundtouch_->receiveSamples(m_sample_buffer, m_nb_);
                if (receive_samples == 0) {
                    m_need_receive_samples_ = true;
                    continue;
                }
            }
            break;
        }
    }
    return receive_samples;
}

/**
 * 开启或关闭录制pcm数据
 * @param flag
 */
void WLAudio::StartStopRecord(bool flag) {
    m_is_record_pcm = flag;
}

/**
 * 音频解码并重采样
 * 得到一个音频包packet的解码pcm数据
 * 一个音频包可能可以解码出来多个Frame
 * 必须得获取到一个frame的pcm数据才退出，否则会一直获取
 */
int WLAudio::_ResampleAudio(void **pcmbuf) {
    int data_size = 0;
    while ((m_play_status != NULL) && !m_play_status->m_is_exit) {
        if (m_play_status->m_seek) {//seek状态，不进行下面的解码
            LOGI("WLAudio seek,please wait...");
            av_usleep(100 * 1000);//100毫秒
            continue;
        }
        if (m_packet_queue->GetQueueSize() == 0) {//加载中
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
        if (m_need_get_packet_) {
            m_packet_queue->GetAVPacket(m_avpacket_);
            pthread_mutex_lock(&m_codec_mutex);
            ret = avcodec_send_packet(m_avcodec_ctx, m_avpacket_);
            if (ret != 0) {
                LOGE("WLAudio avcodec_send_packet failed");
                pthread_mutex_unlock(&m_codec_mutex);
                continue;
            }
        }

        ret = avcodec_receive_frame(m_avcodec_ctx, m_avframe_);
        if (ret == 0) {
            m_need_get_packet_ = false;//获取到解码后的数据,会先返回播放，下次进来再次调用avcodec_receive_frame获取下一个frame
            //2.音频重采样
            if (m_swr_ctx_ == NULL) {
                m_swr_ctx_ = swr_alloc_set_opts(
                        NULL,
                        AV_CH_LAYOUT_STEREO,//输出
                        AV_SAMPLE_FMT_S16,
                        m_avframe_->sample_rate,
                        m_avframe_->channel_layout,//输入
                        (AVSampleFormat) (m_avframe_->format),
                        m_avframe_->sample_rate,
                        NULL, NULL);
                if (swr_init(m_swr_ctx_) < 0) {
                    LOGE("WLAudio swr_init failed!");
                    swr_free(&m_swr_ctx_);
                    m_swr_ctx_ = NULL;
                    pthread_mutex_unlock(&m_codec_mutex);
                    continue;
                }
            }

            /**
             * 重采样，返回单个包的单通道采样点数:对于AAC，一般是1024个采样点
             * 但是有可能m_nb_为0，因为有可能解码后的数据不够一个音频包的数据，所以需要继续获取
             */
            m_nb_ = swr_convert(m_swr_ctx_, &m_convert_buffer_, m_avframe_->nb_samples, (const uint8_t **) (m_avframe_->data), m_avframe_->nb_samples);
            int out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
            data_size = m_nb_ * out_channels * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);//计算得到一个音频包的pcm字节数
            *pcmbuf = m_convert_buffer_;
            pthread_mutex_unlock(&m_codec_mutex);
            break;
        } else {//没有获取到解码后的数据，继续循环获取
            m_need_get_packet_ = true;
            pthread_mutex_unlock(&m_codec_mutex);
            continue;
        }
    }
    return data_size;
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
