#include "wl_video.h"

WLVideo::WLVideo(WLPlayStatus *play_status, CallJava *call_java) {
    this->m_play_status = play_status;
    m_call_java = call_java;
    m_queue = new WLQueue(play_status);
    pthread_mutex_init(&m_codec_mutex, NULL);
}

WLVideo::~WLVideo() {
    pthread_mutex_destroy(&m_codec_mutex);
}

void *playVideo(void *data) {
    WLVideo *video = static_cast<WLVideo *>(data);
    while ((video->m_play_status != NULL) && !video->m_play_status->m_is_exit) {//音频播放结束,m_play_status->isExit会置为true,同时就结束了视频播放
        if (video->m_play_status->m_seek) {//seek时，无需从缓冲区中读取数据
            av_usleep(1000 * 100);
            continue;
        }

        if (video->m_play_status->m_pause) {//暂停时，无需从缓冲区中读取数据
            av_usleep(1000 * 100);
            continue;
        }

        if (video->m_queue->GetQueueSize() == 0) {//视频缓冲区中数据读取完，回调缓冲中，等待数据
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
        AVPacket *av_packet = av_packet_alloc();
        video->m_queue->GetAVPacket(av_packet);

        if (video->m_codec_type == CODEC_MEDIACODEC) {
            if (av_bsf_send_packet(video->m_abs_ctx, av_packet) != 0) {
                av_packet_free(&av_packet);
                continue;
            }
            while (av_bsf_receive_packet(video->m_abs_ctx, av_packet) == 0) {
                double diff = video->GetFrameDiffTime(NULL, av_packet);
                av_usleep(video->GetDelayTime(diff) * 1000000);

                video->m_call_java->OnCallDecodeVPacket(CHILD_THREAD, av_packet->data,av_packet->size);//调用Java层硬解
                av_packet_free(&av_packet);
                continue;
            }
            av_packet = NULL;
        } else if (video->m_codec_type == CODEC_YUV) {
            pthread_mutex_lock(&video->m_codec_mutex);
            if (avcodec_send_packet(video->m_avcodec_ctx, av_packet) != 0) {
                av_packet_free(&av_packet);
                pthread_mutex_unlock(&video->m_codec_mutex);
                continue;
            }

            AVFrame *avframe = av_frame_alloc();
            if (avcodec_receive_frame(video->m_avcodec_ctx, avframe) != 0) {
                av_frame_free(&avframe);
                av_packet_free(&av_packet);
                pthread_mutex_unlock(&video->m_codec_mutex);
                continue;
            }

            //LOGI("子线程解码一个AVFrame成功!");
            if (avframe->format == AV_PIX_FMT_YUV420P) {
                //LOGI("当前视频是YUV420P格式!");
                double diff = video->GetFrameDiffTime(avframe,NULL);//获取音视频的当前时间戳差值进行延迟，控制视频渲染的速度，保证音视频播放对齐
                av_usleep(video->GetDelayTime(diff) * 1000000);
//              av_usleep(diff * 1000000);
                //直接渲染
                video->m_call_java->OnCallRenderYUV(CHILD_THREAD,
                                                    avframe->width,
                                                    avframe->height,
                                                    avframe->linesize[0],
                                                    avframe->data[0],
                                                    avframe->data[1],
                                                    avframe->data[2]);
            } else {
                LOGI("当前视频不是YUV420P格式，需转换!");
                AVFrame *pFrameYUV420p = av_frame_alloc();//分配一个Frame内存空间
                int size = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, video->m_avcodec_ctx->width, video->m_avcodec_ctx->height,1);//获取指定尺寸的YUV420P的内存大小
                uint8_t *buffer = static_cast<uint8_t *>(av_malloc(size * sizeof(uint8_t)));//分配对应的YUV420P大小的内存
                av_image_fill_arrays(pFrameYUV420p->data,
                                     pFrameYUV420p->linesize,
                                     buffer,
                                     AV_PIX_FMT_YUV420P,
                                     video->m_avcodec_ctx->width,
                                     video->m_avcodec_ctx->height,
                                     1);//填充内存和大小到pFrameYUV420p结构中

                SwsContext *sws_ctx = sws_getContext(
                        video->m_avcodec_ctx->width,
                        video->m_avcodec_ctx->height,
                        video->m_avcodec_ctx->pix_fmt,
                        video->m_avcodec_ctx->width,
                        video->m_avcodec_ctx->height,
                        AV_PIX_FMT_YUV420P,
                        SWS_BICUBIC, NULL, NULL, NULL);//只是进行格式转换,非YUV420P格式转换为YUV420P格式
                if (!sws_ctx) {
                    av_frame_free(&pFrameYUV420p);
                    av_free(buffer);
                    pthread_mutex_unlock(&video->m_codec_mutex);
                    continue;
                }

                sws_scale(sws_ctx,
                          avframe->data,
                          avframe->linesize,
                          0,
                          avframe->height,
                          pFrameYUV420p->data,
                          pFrameYUV420p->linesize);//格式转换

                double diff = video->GetFrameDiffTime(pFrameYUV420p, NULL);
                av_usleep(video->GetDelayTime(diff) * 1000000);//获取音视频的当前时间戳差值进行延迟，控制视频渲染的速度，保证音视频播放对齐

                //渲染
                video->m_call_java->OnCallRenderYUV(CHILD_THREAD,
                                                    video->m_avcodec_ctx->width,
                                                    video->m_avcodec_ctx->height,
                                                    pFrameYUV420p->linesize[0],
                                                    pFrameYUV420p->data[0],
                                                    pFrameYUV420p->data[1],
                                                    pFrameYUV420p->data[2]);

                av_frame_free(&pFrameYUV420p);
                av_free(buffer);
                sws_freeContext(sws_ctx);
            }

            av_frame_free(&avframe);
            av_packet_free(&av_packet);
            pthread_mutex_unlock(&video->m_codec_mutex);
        }
    }
//    pthread_exit(&video->m_thread_play);
    return 0;
}

void WLVideo::Play() {
    if ((m_play_status != NULL) && !m_play_status->m_is_exit) {
        pthread_create(&m_thread_play, NULL, playVideo, this);//创建一个视频解码渲染线程，用于从缓冲区获取packet并解码渲染
    }
}

void WLVideo::Release() {
    if (m_queue != NULL) {
        delete m_queue;
        m_queue = NULL;
    }

    pthread_join(m_thread_play, NULL);//等待子线程结束

    if (m_abs_ctx != NULL) {
        av_bsf_free(&m_abs_ctx);
        m_abs_ctx = NULL;
    }

    if (m_avcodec_ctx != NULL) {
        pthread_mutex_lock(&m_codec_mutex);
        avcodec_close(m_avcodec_ctx);
        avcodec_free_context(&m_avcodec_ctx);
        m_avcodec_ctx = NULL;
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
    }
    if (avpacket != NULL) {
        pts = avpacket->pts;
    }

    if (pts == AV_NOPTS_VALUE) {
        pts = 0;
    }
    pts *= av_q2d(m_time_base);//去掉时间基准，单位为秒
    if (pts > 0) {
        m_clock = pts;
    }
    double diff = m_audio->clock - m_clock;//音频-视频，得到差值，这里的audio clock是音频的当前pcm播放时间戳
    //LOGI("audio->clock: %lf, video clock: %lf, diff: %lf", m_audio->clock, m_clock, diff);
    return diff;
}

double WLVideo::GetDelayTime(double diff) {
    //以差值3毫秒为标准进行调整(音视频差值在3毫秒内认为是同步的)  (defaultDelayTime:为正常的视频播放帧率耗时)
    if (diff > 0.003) {//音频快 视频慢，视频减少休眠时间
        m_delay_time = m_delay_time * 2 / 3;
        if (m_delay_time < m_default_delay_time / 2) {
            m_delay_time = m_default_delay_time * 2 / 3;//减少延时
        } else if (m_delay_time > m_default_delay_time * 2) {
            m_delay_time = m_default_delay_time * 2;
        }
    } else if (diff < -0.003) {//视频快，增加休眠时间
        m_delay_time = m_delay_time * 3 / 2;
        if (m_delay_time < m_default_delay_time / 2) {
            m_delay_time = m_default_delay_time * 2 / 3;
        } else if (m_delay_time > m_default_delay_time * 2) {
            m_delay_time = m_default_delay_time * 2;
        }
    } else if (diff == 0.003) {

    }

    if (diff >= 0.5) {//音频太快，视频不休眠
        m_delay_time = 0;
    } else if (diff <= -0.5) {//音频太慢，视频休眠两倍的默认时间
        m_delay_time = m_default_delay_time * 2;
    }

    if (fabs(diff) >= 10) {//相差很大，基本可以确定没有音频，则视频按照帧率进行播放
        m_delay_time = m_default_delay_time;
    }
    //LOGI("m_delay_time is %lf", m_delay_time);
    return m_delay_time;
}
