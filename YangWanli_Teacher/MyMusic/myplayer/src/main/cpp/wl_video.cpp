#include "wl_video.h"

WLVideo::WLVideo(WLPlayStatus *play_status, CallJava *call_java) {
    m_play_status = play_status;
    m_call_java = call_java;
    m_packet_queue = new WLQueue(play_status);
    pthread_mutex_init(&m_codec_mutex, NULL);
}

WLVideo::~WLVideo() {
    pthread_mutex_destroy(&m_codec_mutex);
}

void *_PlayVideo(void *arg) {
    LOGI("WLVideo _PlayVideo in");
    WLVideo *video = static_cast<WLVideo *>(arg);
    while ((video->m_play_status != NULL) && !video->m_play_status->m_is_exit) {//音视频缓冲都无数据时，则会退出
        if (video->m_play_status->m_seek || video->m_play_status->m_pause) {//seek或者是pause时，无需从缓冲区中读取数据
            av_usleep(1000 * 100);
            continue;
        }

        if (video->m_packet_queue->GetQueueSize() == 0) {//视频缓冲区中数据读取完，回调缓冲中，等待数据
            if (!video->m_play_status->m_load) {
                video->m_play_status->m_load = true;
                video->m_call_java->OnCallLoad(CHILD_THREAD, true);
            }
            av_usleep(1000 * 100);
            continue;
        } else {
            if (video->m_play_status->m_load) {
                video->m_play_status->m_load = false;
                video->m_call_java->OnCallLoad(CHILD_THREAD, false);
            }
        }

        AVPacket *avpacket = av_packet_alloc();
        video->m_packet_queue->GetAVPacket(avpacket);
        if (video->m_render_type == RENDER_MEDIACODEC) {
            if (av_bsf_send_packet(video->m_abs_ctx, avpacket) != 0) {
                av_packet_free(&avpacket);
                continue;
            }
            while (av_bsf_receive_packet(video->m_abs_ctx, avpacket) == 0) {
                double diff = video->GetFrameDiffTime(NULL, avpacket);
                av_usleep(video->GetDelayTime(diff) * 1000000);
                video->m_call_java->OnCallDecodeVPacket(CHILD_THREAD, avpacket->data,avpacket->size);
                continue;
            }
            av_packet_free(&avpacket);
        } else if (video->m_render_type == RENDER_YUV) {
            pthread_mutex_lock(&video->m_codec_mutex);
            if (avcodec_send_packet(video->m_avcodec_ctx, avpacket) != 0) {
                av_packet_free(&avpacket);
                pthread_mutex_unlock(&video->m_codec_mutex);
                continue;
            }
            av_packet_free(&avpacket);

            AVFrame *avframe = av_frame_alloc();
            while (avcodec_receive_frame(video->m_avcodec_ctx, avframe) == 0) {
                if (avframe->format == AV_PIX_FMT_YUV420P) {
                    double diff = video->GetFrameDiffTime(avframe,NULL);//获取音视频的当前时间戳差值进行延迟，控制视频渲染的速度，保证音视频播放对齐
                    av_usleep(video->GetDelayTime(diff) * 1000000);
                    video->m_call_java->OnCallRenderYUV(CHILD_THREAD,
                                                        avframe->width,
                                                        avframe->height,
                                                        avframe->linesize[0],
                                                        avframe->data[0],
                                                        avframe->data[1],
                                                        avframe->data[2]);
                } else {
                    AVFrame *scale_avframe = av_frame_alloc();
                    int size = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, video->m_avcodec_ctx->width, video->m_avcodec_ctx->height,1);
                    uint8_t *scale_buffer = static_cast<uint8_t *>(av_malloc(size * sizeof(uint8_t)));
                    av_image_fill_arrays(scale_avframe->data,
                                         scale_avframe->linesize,
                                         scale_buffer,
                                         AV_PIX_FMT_YUV420P,
                                         video->m_avcodec_ctx->width,
                                         video->m_avcodec_ctx->height,
                                         1);
                    SwsContext *sws_ctx = sws_getContext(
                            video->m_avcodec_ctx->width,
                            video->m_avcodec_ctx->height,
                            video->m_avcodec_ctx->pix_fmt,
                            video->m_avcodec_ctx->width,
                            video->m_avcodec_ctx->height,
                            AV_PIX_FMT_YUV420P,
                            SWS_BICUBIC, NULL, NULL, NULL);
                    if (!sws_ctx) {
                        av_frame_free(&scale_avframe);
                        av_free(scale_buffer);
                        pthread_mutex_unlock(&video->m_codec_mutex);
                        continue;
                    }
                    sws_scale(sws_ctx,
                              avframe->data,
                              avframe->linesize,
                              0,
                              avframe->height,
                              scale_avframe->data,
                              scale_avframe->linesize);

                    double diff = video->GetFrameDiffTime(scale_avframe, NULL);
                    av_usleep(video->GetDelayTime(diff) * 1000000);//获取音视频的当前时间戳差值进行延迟，控制视频渲染的速度，保证音视频播放对齐
                    video->m_call_java->OnCallRenderYUV(CHILD_THREAD,
                                                        video->m_avcodec_ctx->width,
                                                        video->m_avcodec_ctx->height,
                                                        scale_avframe->linesize[0],
                                                        scale_avframe->data[0],
                                                        scale_avframe->data[1],
                                                        scale_avframe->data[2]);
                    av_frame_free(&scale_avframe);
                    av_free(scale_buffer);
                    sws_freeContext(sws_ctx);
                }
            }
            av_frame_free(&avframe);
            pthread_mutex_unlock(&video->m_codec_mutex);
        }
    }
//    pthread_exit(&video->m_thread_play);
    LOGI("WLVideo _PlayVideo out");
    return 0;
}

void WLVideo::Play() {
    if ((m_play_status != NULL) && !m_play_status->m_is_exit) {
        int ret = pthread_create(&m_play_thread_, NULL, _PlayVideo, this);
        if (ret != 0) {
            LOGE("Create Play Video Thread failed!");
        }
    }
}

void WLVideo::Release() {
    if (m_packet_queue != NULL) {
        delete m_packet_queue;
        m_packet_queue = NULL;
    }
    void *thread_ret;
    int result = pthread_join(m_play_thread_, &thread_ret);
    LOGI("video m_play_thread_ join result: %d", result);
    if (result != 0) {
        switch (result) {
            case ESRCH:
                LOGE("video m_play_thread_ pthread_join failed: Thread not found (ESRCH)");
                break;
            case EINVAL:
                LOGE("video m_play_thread_ pthread_join failed: Invalid thread or thread already detached (EINVAL)");
                break;
            case EDEADLK:
                LOGE("video m_play_thread_ pthread_join failed: Deadlock detected (EDEADLK)");
                break;
            default:
                LOGE("video m_play_thread_ pthread_join failed: Unknown error (%d)", result);
        }
        // Exit or perform additional actions if needed
        exit(EXIT_FAILURE);
    } else {
        LOGI("video m_play_thread_ Thread returned: %ld", (long)thread_ret);
    }

    if (m_abs_ctx != NULL) {
        av_bsf_free(&m_abs_ctx);
    }
    if (m_avcodec_ctx != NULL) {
        pthread_mutex_lock(&m_codec_mutex);
        /**
         * 释放解码器上下文
         avcodec_close()函数用于关闭解码器，释放解码器上下文，但是不会释放AVCodec结构体。
         但是在ffmpeg 4.0版本中，avcodec_close()函数已经被废弃，不再使用。
         */
//        avcodec_close(m_avcodec_ctx);
        avcodec_free_context(&m_avcodec_ctx);
        pthread_mutex_unlock(&m_codec_mutex);
    }
    if (m_play_status != NULL) {
        m_play_status = NULL;
    }
    if (m_call_java != NULL) {
        m_call_java = NULL;
    }
}

double WLVideo::GetFrameDiffTime(AVFrame *avframe, AVPacket *avpacket) {
    double pts = 0;
    if (avframe != NULL) {
        pts = av_frame_get_best_effort_timestamp(avframe);
    } else if (avpacket != NULL) {
        pts = avpacket->pts;
    }

    if (pts == AV_NOPTS_VALUE) {
        pts = 0;
    }
    pts *= av_q2d(m_time_base);//去掉时间基准，单位为秒
    if (pts > 0) {
        m_clock = pts;
    }

    LOGI("audio clock diff is %d", (int)((m_audio->clock - m_last_audio_clock_) * 1000));
    m_last_audio_clock_ = m_audio->clock;
    double diff = m_audio->clock - m_clock;//音频-视频，得到差值，这里的audio clock是音频的当前pcm播放时间戳
    LOGI("audio->clock: %lf, video clock: %lf, diff: %lf", m_audio->clock, m_clock, diff);
    return diff;
}

double WLVideo::GetDelayTime(double diff) {
    //以差值3毫秒为标准进行调整(音视频差值在3毫秒内认为是同步的)  (defaultDelayTime:为正常的视频播放帧率耗时)
    if (diff > 0.003) {//音频快 视频慢，视频减少休眠时间
        m_delay_time_ = m_delay_time_ * 2 / 3;
        if (m_delay_time_ < m_default_delay_time / 2) {
            m_delay_time_ = m_default_delay_time * 2 / 3;//减少延时
        } else if (m_delay_time_ > m_default_delay_time * 2) {
            m_delay_time_ = m_default_delay_time * 2;
        }
    } else if (diff < -0.003) {//视频快，增加休眠时间
        m_delay_time_ = m_delay_time_ * 3 / 2;
        if (m_delay_time_ < m_default_delay_time / 2) {
            m_delay_time_ = m_default_delay_time * 2 / 3;
        } else if (m_delay_time_ > m_default_delay_time * 2) {
            m_delay_time_ = m_default_delay_time * 2;
        }
    } else if (diff == 0.003) {

    }

    if (diff >= 0.5) {//音频太快，视频不休眠
        m_delay_time_ = 0;
    } else if (diff <= -0.5) {//音频太慢，视频休眠两倍的默认时间
        m_delay_time_ = m_default_delay_time * 2;
    }

    if (fabs(diff) >= 10) {//相差很大，基本可以确定没有音频，则视频按照帧率进行播放
        m_delay_time_ = m_default_delay_time;
    }
    //LOGI("m_delay_time_ is %lf", m_delay_time_);
    return m_delay_time_;
}
