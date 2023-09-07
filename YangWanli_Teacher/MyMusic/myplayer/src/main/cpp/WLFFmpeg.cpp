#include "WLFFmpeg.h"
#include <iostream>
#include <iomanip>

WLFFmpeg::WLFFmpeg(WLPlayStatus *playStatus, CallJava *calljava, const char *url) {
    this->playStatus = playStatus;
    this->callJava = calljava;
    strcpy(this->url, url);
    isExit = false;
    pthread_mutex_init(&init_mutex, NULL);
    pthread_mutex_init(&seek_mutex, NULL);
}

WLFFmpeg::~WLFFmpeg() {
    pthread_mutex_destroy(&init_mutex);
    pthread_mutex_destroy(&seek_mutex);
}

/*
 * 若在avformat_open_input调用时加载网络流很慢，导致阻塞卡死，这个回调函数中返回AVERROR_EOF
 * 会立即退出加载，返回失败
 * */
int avformat_callback(void *ctx) {
    WLFFmpeg *wlfFmpeg = (WLFFmpeg *) (ctx);
    if (wlfFmpeg->playStatus->isExit) {
        return AVERROR_EOF;
    }
    return 0;
}

void WLFFmpeg::demuxFFmpegThread() {
    pthread_mutex_lock(&init_mutex);

    av_register_all();
    avformat_network_init();

    pFormatCtx = avformat_alloc_context();
    pFormatCtx->interrupt_callback.callback = avformat_callback;
    pFormatCtx->interrupt_callback.opaque = this;
    if (avformat_open_input(&pFormatCtx, url, NULL, NULL) != 0) {
        if (LOG_DEBUG) {
            LOGE("can not open url: %s", url);
            callJava->onCallError(CHILD_THREAD, 1001, "can not open url");
        }
        isExit = true;
        pthread_mutex_unlock(&init_mutex);
        return;
    }
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        if (LOG_DEBUG) {
            LOGE("can not find streams from url: %s", url);
            callJava->onCallError(CHILD_THREAD, 1002, "can not find streams from url");
        }
        isExit = true;
        pthread_mutex_unlock(&init_mutex);
        return;
    }

    for (int i = 0; i < pFormatCtx->nb_streams; ++i) {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            if (pWLAudio == NULL) {
                pWLAudio = new WLAudio(playStatus, pFormatCtx->streams[i]->codecpar->sample_rate, callJava);//创建音频播放类实例
                pWLAudio->streamIndex = i;
                pWLAudio->codecPar = pFormatCtx->streams[i]->codecpar;
                pWLAudio->duration = pFormatCtx->duration / AV_TIME_BASE;//媒体总时长，单位为秒
                pWLAudio->time_base = pFormatCtx->streams[i]->time_base;
                duration = pWLAudio->duration;

                callJava->onCallPcmRate(CHILD_THREAD, pWLAudio->sample_Rate, 16, 2);//上报音频采样率，采样位宽，和声道数信息
            }
        } else if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (pWLVideo == NULL) {
                pWLVideo = new WLVideo(playStatus, callJava);//创建视频播放类实例
                pWLVideo->streamIndex = i;
                pWLVideo->codecPar = pFormatCtx->streams[i]->codecpar;
                pWLVideo->time_base = pFormatCtx->streams[i]->time_base;

                int num = pFormatCtx->streams[i]->avg_frame_rate.num;
                int den = pFormatCtx->streams[i]->avg_frame_rate.den;
                if ((num != 0) && (den != 0)) {//获取到平均帧率值
                    int fps = num / den;//比如25/1
                    pWLVideo->defaultDelayTime = 1.0 / fps;//根据帧率值计算得到每一帧的播放延时
                    LOGI("fps %d, defaultDelayTime: %lf", fps, pWLVideo->defaultDelayTime);
                }
            }
        }
    }

    //打开ffmpeg音频解码器
    if (pWLAudio != NULL) {
        getCodecContext(pWLAudio->codecPar, &pWLAudio->avCodecContext);
    }

    //打开ffmpeg视频解码器
    if (pWLVideo != NULL) {
        getCodecContext(pWLVideo->codecPar, &pWLVideo->avCodecContext);
    }

    if (callJava != NULL) {
        if ((playStatus != NULL) && !playStatus->isExit) {
            callJava->onCallPrepared(CHILD_THREAD);//回调已准备好资源
        } else {
            isExit = true;
        }
    }
    pthread_mutex_unlock(&init_mutex);
}

void *demuxFFmpeg(void *data) {
    WLFFmpeg *wlfFmpeg = (WLFFmpeg *) (data);
    wlfFmpeg->demuxFFmpegThread();

    /*
     * 使用return语句退出线程比使用pthread_exit()函数更简单和直观。
     * 一般情况下，如果只需要退出当前线程，
     * 而不需要精确的控制，使用return语句是更常见和推荐的做法。
     * 只有在需要在任意位置立即终止线程执行的特殊情况下，才需要使用pthread_exit()函数。
     * */
//    pthread_exit(&wlfFmpeg->demuxThread);
    return 0;
}

void WLFFmpeg::prepared() {
    pthread_create(&demuxThread, NULL, demuxFFmpeg, this);
}

void WLFFmpeg::startFFmpegThread() {
    if (pWLAudio == NULL) {
        if (LOG_DEBUG) {
            LOGE("audio is NULL");
            callJava->onCallError(CHILD_THREAD, 1007, "audio is NULL");
        }
        return;
    }

    if (pWLVideo == NULL) {//目前要求必须要有视频流
        return;
    }

    supportMediaCodec = false;
    pWLVideo->audio = pWLAudio;//将音频播放对象设置到视频播放对象中，用于获取音频参数进行音视频时间戳同步操作
    const char *codecName = (pWLVideo->avCodecContext->codec)->name;
    LOGI("WLFFmpeg start codecName: %s", codecName);
    if (supportMediaCodec = callJava->onCallIsSupportVideo(CHILD_THREAD, codecName)) {//回调Java函数，支持硬解，优先使用硬解
        LOGI("当前设备支持硬解码当前视频!!!");
        /*
         * 对于硬解视频，必须传入的码流头是annexb格式，所以需要转换数据，添加annexb格式头
         * */
        const AVBitStreamFilter * bsFilter = NULL;
        if (strcasecmp(codecName, "h264") == 0) {
            bsFilter = av_bsf_get_by_name("h264_mp4toannexb");
        } else if (strcasecmp(codecName, "hevc") == 0) {
            bsFilter = av_bsf_get_by_name("hevc_mp4toannexb");
        }
        if (bsFilter == NULL) {
            goto end;
        }
        if (av_bsf_alloc(bsFilter, &pWLVideo->abs_ctx) != 0) {
            supportMediaCodec = false;
            goto end;
        }
        if (avcodec_parameters_copy(pWLVideo->abs_ctx->par_in, pWLVideo->codecPar) < 0) {
            supportMediaCodec = false;
            av_bsf_free(&pWLVideo->abs_ctx);
            pWLVideo->abs_ctx = NULL;
            goto end;
        }
        if (av_bsf_init(pWLVideo->abs_ctx) != 0) {
            supportMediaCodec = false;
            av_bsf_free(&pWLVideo->abs_ctx);
            pWLVideo->abs_ctx = NULL;
            goto end;
        }
        pWLVideo->abs_ctx->time_base_in = pWLVideo->time_base;//时间基准
    }

    end:
    if (supportMediaCodec) {
        pWLVideo->codectype = CODEC_MEDIACODEC;
        /*
         * 回调Java方法，传递ffmepg的extradata数据，用来初始化硬件解码器，
         * */
//        for (int i = 0;i < pWLVideo->avCodecContext->extradata_size; i++) {
//            LOGI("%02X", pWLVideo->avCodecContext->extradata[i]);
//        }
        LOGI("native onCallinitMediaCodec extradata size: %d", pWLVideo->avCodecContext->extradata_size);
        int size = pWLVideo->avCodecContext->extradata_size;
        char output[4];
        char buffer[1024] = {0};
        for (size_t i = 0; i < size; ++i) {
            sprintf(output, "%02X ", pWLVideo->avCodecContext->extradata[i]);
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

        pWLVideo->callJava->onCallinitMediaCodec(CHILD_THREAD,
                                                 codecName,
                                                 pWLVideo->avCodecContext->width,
                                                 pWLVideo->avCodecContext->height,
                                                 pWLVideo->avCodecContext->extradata_size,
                                                 pWLVideo->avCodecContext->extradata);
    }

    pWLAudio->play();//开启音频播放，内部创建子线程用于获取缓冲区的pacekt，解码为pcm并给到opengles播放
    pWLVideo->play();//开启视频播放,内部创建子线程用于获取缓冲区的pacekt,然后进行解码渲染

    LOGI("WLFFmpeg is start");
    int count = 0;
    while ((playStatus != NULL) && !playStatus->isExit) {
        if (playStatus->seek) {//seek状态时，不往下读取
            av_usleep(100 * 1000);//100毫秒
            LOGI("now is seek continue");
            continue;
        }
        /*对于ape音频文件，一个音频packet可以解码为多个frame，因此需要减少缓冲区packet的个数，
         * 避免seek时卡顿,但是对于一个packet对应一个frame的音频文件，这里要改为40
         */
        if (pWLAudio->queue->getQueueSize() > 40) {//这里控制一下读取包的速度，音频包缓冲队列存储的数据不宜过多，不往下读取
            av_usleep(100 * 1000);//100毫秒
            continue;
        }

        AVPacket *avPacket = av_packet_alloc();//分配packet内存
        pthread_mutex_lock(&seek_mutex);
        int ret = av_read_frame(pFormatCtx, avPacket);//读取媒体文件的packet
        pthread_mutex_unlock(&seek_mutex);
        if (ret == 0) {
            if (avPacket->stream_index == pWLAudio->streamIndex) {
                count++;
                if (LOG_DEBUG) {
//                    LOGI("read audio the packet: %d", count);
                }
                pWLAudio->queue->putAVPacket(avPacket);
            } else if (avPacket->stream_index == pWLVideo->streamIndex) {
                pWLVideo->queue->putAVPacket(avPacket);
            } else {//非音频packet
                av_packet_free(&avPacket);
                av_free(avPacket);
            }
        } else {//读取到文件尾，等待缓冲区中的数据消耗完
            av_packet_free(&avPacket);
            av_free(avPacket);
            while ((playStatus != NULL) && !playStatus->isExit) {
                if (pWLAudio->queue->getQueueSize() > 0) {//音频缓冲区中的packet未消耗完，则处于线程延迟中，等待音频播放线程消耗
                    av_usleep(100 * 1000);//100毫秒
                    continue;
                } else {//音频缓冲区中的packet已消耗完
                    if (!playStatus->seek) {
                        av_usleep(100 * 1000);
                        playStatus->isExit = true;
                        LOGI("playStatus isExit set true");
                    }
                    break;
                }
            }
        }
    }

    /*回调整个播放已完成
     按上面的逻辑，是以音频播放结束为准
     */
    if (callJava != NULL) {
        callJava->onCallComplete(CHILD_THREAD);
    }

    isExit = true;
    if (LOG_DEBUG) {
        LOGI("get packet is over");
    }
}

void *startFFmpeg(void *data) {
    WLFFmpeg *wlfFmpeg = (WLFFmpeg *) (data);
    wlfFmpeg->startFFmpegThread();

    /*
     * 使用return语句退出线程比使用pthread_exit()函数更简单和直观。
     * 一般情况下，如果只需要退出当前线程，
     * 而不需要精确的控制，使用return语句是更常见和推荐的做法。
     * 只有在需要在任意位置立即终止线程执行的特殊情况下，才需要使用pthread_exit()函数。
     * */
//    pthread_exit(&wlfFmpeg->demuxThread);
    return 0;
}

void WLFFmpeg::start() {
    pthread_create(&startThread, NULL, startFFmpeg, this);//开启一个子线程，用于读取流数据，并存放到缓存队列中
}

void WLFFmpeg::pause() {
    if (playStatus != NULL) {
        playStatus->pause = true;
    }

    if (pWLAudio != NULL) {
        pWLAudio->pause();
    }
}

void WLFFmpeg::resume() {
    if (playStatus != NULL) {
        playStatus->pause = false;
    }

    if (pWLAudio != NULL) {
        pWLAudio->resume();
    }
}

void WLFFmpeg::seek(int64_t secds) {
    LOGI("WLFFmpeg seek secds: %lld", secds);
    if (duration <= 0) {
        return;
    }
    if ((secds >= 0) && (secds <= duration)) {
        playStatus->seek = true;//设置为seek状态
        pthread_mutex_lock(&seek_mutex);
        int64_t rel = secds * AV_TIME_BASE;
        avformat_seek_file(pFormatCtx, -1, INT64_MIN, rel, INT64_MAX, 0);//seek到指定的时间点，这里没有指定某个流进行seek，由ffmpeg内部去判断
        if (pWLAudio != NULL) {
            pWLAudio->queue->clearAvPacket();
            pWLAudio->clock = 0;
            pWLAudio->last_time = 0;
            pthread_mutex_lock(&pWLAudio->codecMutex);
            avcodec_flush_buffers(pWLAudio->avCodecContext);//清空解码器内部缓冲
            pthread_mutex_unlock(&pWLAudio->codecMutex);
            LOGI("WLFFmpeg pWLAudio seek!!! ");
        }

        if (pWLVideo != NULL) {
            pWLVideo->queue->clearAvPacket();
            pWLVideo->clock = 0;
            pthread_mutex_lock(&pWLVideo->codecMutex);
            avcodec_flush_buffers(pWLVideo->avCodecContext);
            pthread_mutex_unlock(&pWLVideo->codecMutex);
            LOGI("WLFFmpeg pWLVideo seek!!! ");
        }
        pthread_mutex_unlock(&seek_mutex);
        playStatus->seek = false;
        LOGI("WLFFmpeg seek end!");
    }
}

void WLFFmpeg::release() {
    LOGI("WLFFmpeg release in");
    playStatus->isExit = true;

    pthread_join(demuxThread, NULL);//等待子线程结束
    pthread_join(startThread, NULL);//等待子线程结束

    pthread_mutex_lock(&init_mutex);
    int sleepCount = 0;
    while (!isExit) {//若播放子线程仍然没有退出，则延迟等待10s
        if (sleepCount > 1000) {
            isExit = true;
        }
        if (LOG_DEBUG) {
            LOGI("wait ffmpeg exit %d", sleepCount);
        }
        sleepCount++;
        av_usleep(1000 * 10);//10ms
    }

    if (pWLAudio != NULL) {
        pWLAudio->release();
        delete pWLAudio;
        pWLAudio = NULL;
        LOGI("WLFFmpeg release pWLAudio");
    }

    if (pWLVideo != NULL) {
        pWLVideo->release();
        delete pWLVideo;
        pWLVideo = NULL;
        LOGI("WLFFmpeg release pWLVideo");
    }

    LOGI("WLFFmpeg release pFormatCtx");
    if (pFormatCtx != NULL) {
        avformat_close_input(&pFormatCtx);
        avformat_free_context(pFormatCtx);
        pFormatCtx = NULL;
    }

    if (playStatus != NULL) {
        playStatus = NULL;
    }
    if (callJava != NULL) {
        callJava = NULL;
    }
    pthread_mutex_unlock(&init_mutex);
    LOGI("WLFFmpeg release end");
}

void WLFFmpeg::setVolume(int percent) {
    if (pWLAudio != NULL) {
        pWLAudio->setVolume(percent);
    }
}

void WLFFmpeg::setMute(int mute) {
    if (pWLAudio != NULL) {
        pWLAudio->setMute(mute);
    }
}

void WLFFmpeg::setPitch(float pitch) {
    if (pWLAudio != NULL) {
        pWLAudio->setPitch(pitch);
    }
}

void WLFFmpeg::setSpeed(float speed) {
    if (pWLAudio != NULL) {
        pWLAudio->setSpeed(speed);
    }
}

int WLFFmpeg::getSampleRate() {
    if (pWLAudio != NULL) {
        return pWLAudio->avCodecContext->sample_rate;
    }
    return 0;
}

void WLFFmpeg::startStopRecord(bool start) {
    if (pWLAudio != NULL) {
        pWLAudio->startStopRecord(start);
    }
}

bool WLFFmpeg::cutAudioPlay(int start_time, int end_time, bool showPcm) {
    if ((start_time >= 0) && (end_time <= duration) && (start_time < end_time)) {//符合裁剪条件
        pWLAudio->isCut = true;
        pWLAudio->end_time = end_time;
        pWLAudio->showPcm = showPcm;

        seek(start_time);
        return true;
    }
    return false;
}

int WLFFmpeg::getCodecContext(AVCodecParameters *codecPar, AVCodecContext **avCodecContext) {
    AVCodec *avCodec = avcodec_find_decoder(codecPar->codec_id);
    if (!avCodec) {
        if (LOG_DEBUG) {
            LOGE("can not find deocder");
        }
        callJava->onCallError(CHILD_THREAD, 1003, "can not find deocder");
        isExit = true;
        pthread_mutex_unlock(&init_mutex);
        return -1;
    }

    *avCodecContext = avcodec_alloc_context3(avCodec);
    if (!*avCodecContext) {
        if (LOG_DEBUG) {
            LOGE("can not alloc new decoderCtx");
        }
        callJava->onCallError(CHILD_THREAD, 1004, "can not alloc new decoderCtx");
        isExit = true;
        pthread_mutex_unlock(&init_mutex);
        return -1;
    }

    if (avcodec_parameters_to_context(*avCodecContext, codecPar) < 0) {
        if (LOG_DEBUG) {
            LOGE("can not fill decoderCtx");
        }
        callJava->onCallError(CHILD_THREAD, 1005, "can not fill decoderCtx");
        isExit = true;
        pthread_mutex_unlock(&init_mutex);
        return -1;
    }

    if (avcodec_open2(*avCodecContext, avCodec, 0) != 0) {
        if (LOG_DEBUG) {
            LOGE("can not open audio decoder");
        }
        callJava->onCallError(CHILD_THREAD, 1006, "can not open audio decoder");
        isExit = true;
        pthread_mutex_unlock(&init_mutex);
        return -1;
    }
    LOGI("decoder open success!");
    return 0;
}
