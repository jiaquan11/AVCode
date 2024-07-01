#include "wl_video.h"
#include "h264_sps_parser.h"
#include "hevc_sps_parser.h"

WLVideo::WLVideo(WLPlayStatus *play_status, CallJava *call_java) {
    m_play_status = play_status;
    m_call_java = call_java;
    m_packet_queue = new WLQueue(play_status);
    m_pts_queue = new WLLinkOrderQueue();
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
        if (video->m_render_type == RENDER_MEDIACODEC) {
            if (!video->m_read_frame_finished && (video->m_pts_queue->Size() <= video->m_max_ref_frames)) {
                LOGI("video wait pts queue size is %d max_ref_frames: %d", video->m_pts_queue->Size(), video->m_max_ref_frames);
                av_usleep(5 * 1000);//休眠5毫秒
                continue;
            }

            video->m_packet_queue->GetAVPacket(avpacket);
            if (av_bsf_send_packet(video->m_abs_ctx, avpacket) != 0) {
                LOGE("video av_bsf_send_packet to filter failed");
                av_packet_free(&avpacket);
                continue;
            }
            /**
             * av_bsf_receive_packet,目前验证是没有缓冲的，所以这里不需要循环接收
             */
            if (av_bsf_receive_packet(video->m_abs_ctx, avpacket) == 0) {
                int pts_ms = video->m_pts_queue->Popup();
                double diff = video->GetFrameDiffTime(pts_ms);
                av_usleep(video->GetDelayTime(diff) * 1000000);
                video->m_call_java->OnCallDecodeVPacket(CHILD_THREAD, avpacket->data,avpacket->size, avpacket->pts * av_q2d(video->m_time_base));
            } else {
                LOGE("video av_bsf_receive_packet from filter failed");
                av_packet_free(&avpacket);
                continue;
            }

        } else if (video->m_render_type == RENDER_YUV) {
            video->m_packet_queue->GetAVPacket(avpacket);
            pthread_mutex_lock(&video->m_codec_mutex);
            if (avcodec_send_packet(video->m_avcodec_ctx, avpacket) != 0) {
                LOGE("WLVideo avcodec_send_packet failed");
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
                    //手动复制时间戳和其他元数据,用于音视频同步
                    scale_avframe->pts = avframe->pts;
                    scale_avframe->pkt_dts = avframe->pkt_dts;
                    scale_avframe->best_effort_timestamp = av_frame_get_best_effort_timestamp(avframe);
                    scale_avframe->sample_aspect_ratio = avframe->sample_aspect_ratio;
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
    if (m_pts_queue != NULL) {
        delete m_pts_queue;
        m_pts_queue = NULL;
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

double WLVideo::GetFrameDiffTime(int pts_ms) {
    if (pts_ms < 0) {
        return 0;
    }
    LOGI("GetFrameDiffTime pts_ms: %d", pts_ms);
    m_clock = pts_ms * 1.0 / 1000;
    LOGI("audio clock diff is %d", (int)((m_audio->clock - m_last_audio_clock_) * 1000));
    m_last_audio_clock_ = m_audio->clock;
    LOGI("video clock diff is %d", (int)((m_clock - m_last_video_clock_) * 1000));
    m_last_video_clock_ = m_clock;
    double diff = m_audio->clock - m_clock;//音频-视频，得到差值，这里的audio clock是音频的当前pcm播放实际计算的时间戳
    LOGI("audio->clock: %lf, video clock: %lf, diff: %lf", m_audio->clock, m_clock, diff);
    return diff;
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
    if (pts >= 0) {
        m_clock = pts;
    }
    LOGI("audio clock diff is %d", (int)((m_audio->clock - m_last_audio_clock_) * 1000));
    m_last_audio_clock_ = m_audio->clock;
    LOGI("video clock diff is %d", (int)((m_clock - m_last_video_clock_) * 1000));
    m_last_video_clock_ = m_clock;
    double diff = m_audio->clock - m_clock;//音频-视频，得到差值，这里的audio clock是音频的当前pcm播放时间戳
    LOGI("audio->clock: %lf, video clock: %lf, diff: %lf", m_audio->clock, m_clock, diff);
    return diff;
}

double WLVideo::GetDelayTime(double diff_secds) {
    //以差值3毫秒为标准进行调整(音视频差值在3毫秒内认为是同步的)  (defaultDelayTime:为正常的视频播放帧率耗时)
    if (diff_secds > 0.003) {//音频快 视频慢，视频减少休眠时间
        m_delay_time_ = m_delay_time_ * 2 / 3;
        if (m_delay_time_ < m_default_delay_time / 2) {
            m_delay_time_ = m_default_delay_time * 2 / 3;//减少延时
        } else if (m_delay_time_ > m_default_delay_time * 2) {
            m_delay_time_ = m_default_delay_time * 2;
        }
    } else if (diff_secds < -0.003) {//视频快，增加休眠时间
        m_delay_time_ = m_delay_time_ * 3 / 2;
        if (m_delay_time_ < m_default_delay_time / 2) {
            m_delay_time_ = m_default_delay_time * 2 / 3;
        } else if (m_delay_time_ > m_default_delay_time * 2) {
            m_delay_time_ = m_default_delay_time * 2;
        }
    } else if (diff_secds == 0.003) {

    }

    if (diff_secds >= 0.5) {//音频太快，视频不休眠
        m_delay_time_ = 0;
    } else if (diff_secds <= -0.5) {//音频太慢，视频休眠两倍的默认时间
        m_delay_time_ = m_default_delay_time * 2;
    }

    if (fabs(diff_secds) >= 10) {//相差很大，基本可以确定没有音频，则视频按照帧率进行播放
        m_delay_time_ = m_default_delay_time;
    }
    LOGI("m_delay_time_ is %lf", m_delay_time_);
    return m_delay_time_;
}

/**
 * 从AVCodecContext中extradata里面提取H264解码信息(SPS,PPS)
 */
void WLVideo::Get264Params(AVCodecContext *avctx) {
    //已在SPS和PPS前面添加了四字节的startcode(00 00 00 01)
    unsigned char head[] = {0, 0, 0, 1};
    m_sps_len_ = avctx->extradata[7] - avctx->extradata[6];//提取得到SPS的长度
    m_sps_ = new char[m_sps_len_ + 1 + 4];
    memcpy(m_sps_, head, 4);//SPS前面添加startcode(00 00 00 01)
    memcpy(m_sps_ + 4, &avctx->extradata[8], m_sps_len_);//从extradata第8个字节拷贝SPS实际数据

    //SPS数据后有一个字节的number of PPS NALUS(usually 1),这里直接跳过了
    m_pps_len = avctx->extradata[8 + m_sps_len_ + 2] - avctx->extradata[8 + m_sps_len_ + 1];//提取得到PPS的长度
    m_pps_ = new char[m_pps_len + 1 + 4];
    memcpy(m_pps_, head, 4);//PPS前面添加startcode(00 00 00 01)
    memcpy(m_pps_ + 4, &avctx->extradata[8 + m_sps_len_ + 3], m_pps_len);//从extradata第(8 + mSpsLen + 3)字节后拷贝PPS实际数据
    m_sps_len_ += 4;
    m_sps_[m_sps_len_] = '\0';
    m_pps_len += 4;
    m_pps_[m_pps_len] = '\0';
    LOGI("WLVideo Get264Params m_sps_len_ :%d, m_pps_len:%d", m_sps_len_, m_pps_len);

    int level = 0;
    int profile = 0;
    bool interlaced = 0;
    int max_ref_frames = 0;
    /**
     * 从SPS中解析出level, profile, interlaced, maxRefFrames等信息
     * 从SPS中可以获取编码流中的最大参考帧数
     * 这里解析SPS,输入的数据偏移前面四字节的startcode及一个字节的Nail头
     */
    parseh264_sps((uint8_t *) (m_sps_ + 4 + 1), m_sps_len_ - 4 - 1, &level, &profile, &interlaced, &max_ref_frames);
    m_max_ref_frames = max_ref_frames;
    LOGI("WLVideo Get264Params level:%d profile:%d interlaced:%d max_ref_frames:%d", level, profile, interlaced, max_ref_frames);
}

/**
 * 从AVCodecContext中extradata里面提取H265解码信息(VPS,SPS,PPS)
 */
void WLVideo::Get265Params(AVCodecContext *avctx) {
    LOGI("WLVideo Get265Params in");
    //已在VPS,SPS和PPS前面添加了四字节的startcode(00 00 00 01)
    unsigned char head[] = {0, 0, 0, 1};
    //目前发现苹果录制的265视频，可能会有多个PPS，因此此次分配大小内存要大些
    m_pps_ = new char[kMaxPpsLen];
    memset(m_pps_, 0, kMaxPpsLen);

    //HEVC extradata前0-21个字节数据直接跳过
    int last_nal_unit_len = 0;
    int num_arrays = avctx->extradata[22];//表示extradata中存放NALU单元类型的数量，比如03:就表示VPS SPS PPS
    uint8_t *extra_data = avctx->extradata;
    LOGI("WLVideo Get265Params numArrays: %d", num_arrays);
    int offset = 0;
    for (int j = 0; j < num_arrays; j++) {//遍历各个NALU单元
        int current_nal_offset = offset;
        int array_completeness = (extra_data[23 + offset] & 0x80) >> 7;//取字节的最高位  没什么用处
        unsigned int reserved = (extra_data[23 + offset] & 0x40) >> 6;//取字节的高第2位  没什么用处
        unsigned int nal_unit_type = extra_data[23 + offset] & 0x3f;//取字节的低6位 H265 nal_unit_type占6位
        offset++;//偏移一个字节
        unsigned int num_nalus = (extra_data[23 + offset] << 8) + (extra_data[23 + offset + 1]);//两个字节表示某个NALU单元的数量
        LOGI("WLVideo Get265Params numNalus: %d", num_nalus);
        offset += 2;//偏移两个字节
        for (int i = 0; i < num_nalus; i++) {
            unsigned int nal_unit_len = (extra_data[23 + offset] << 8) + (extra_data[23 + offset + 1]);//这个表示单个NALU单元的size
            offset += 2;
            offset += nal_unit_len;//这里offset已偏移单个NALU单元结束
            LOGI("WLVideo Get265Params nal_unit_len: %d", nal_unit_len);
            if (i > 0) {//同一个类型的NALU单元有多个的情况  若是两个同类型的NALU，第二个NALU紧跟的就是：NALU长度(两个字节)+实际数据
                current_nal_offset += (last_nal_unit_len + 2);
            } else {//同一类型的NALU单元只有一个
                current_nal_offset += 5;//currentNalOffset偏移5个字节，其实指向的是实际的NALU数据开始处
            }
            last_nal_unit_len = nal_unit_len;

            int nal_len = offset - current_nal_offset;//这里得到的是NALU数据长度，其实应该就是nalUnitLength
            LOGI("WLVideo Get265Params nal_unit_type = %d offset = %d, current_nal_offset = %d,nalLen = %d array_completeness: %d reserved: %d",
                 nal_unit_type, offset, current_nal_offset, nal_len, array_completeness, reserved);
            if ((nal_unit_type == 32) && (nal_len > 0)) {//VPS
                m_vps_ = new char[nal_len + 4];
                m_vps_len_ = nal_len + 4;
                memcpy(m_vps_, head, 4);
                memcpy(m_vps_ + 4, (char *) extra_data + 23 + current_nal_offset, nal_len);
            } else if ((nal_unit_type == 33) && (nal_len > 0)) {//SPS
                m_sps_ = new char[nal_len + 4];
                m_sps_len_ = nal_len + 4;
                memcpy(m_sps_, head, 4);
                memcpy(m_sps_ + 4, (char *) extra_data + 23 + current_nal_offset, nal_len);
            } else if ((nal_unit_type == 34) && (nal_len > 0)) {//PPS,可能有多个
                char *tmp_buf = new char[nal_len + 4];
                memset(tmp_buf, 0, nal_len + 4);
                memcpy(tmp_buf, head, 4);
                memcpy(tmp_buf + 4, (char *) extra_data + 23 + current_nal_offset, nal_len);
                memcpy(m_pps_ + m_pps_len, tmp_buf, nal_len + 4);
                m_pps_len += (nal_len + 4);
                delete []tmp_buf;
                tmp_buf = NULL;
            } else if ((nal_unit_type == 39) && (nal_len > 0)) {
                //preSEI
            } else if ((nal_unit_type == 40) && (nal_len > 0)) {
                //sufSEI
            }
        }
    }
    LOGI("WLVideo Get265Params m_vps_len_: %d,m_sps_len_: %d,m_pps_len: %d", m_vps_len_, m_sps_len_, m_pps_len);

    HevcSPSParams params;
    ParseHEVCSPS((uint8_t*)(m_sps_ + 4), m_sps_len_ - 4, params);
    m_max_ref_frames = params.max_ref_frames;
    LOGI("WLVideo Get265Params max_ref_frames: %d", params.max_ref_frames);
    LOGI("WLVideo Get265Params out");
}
