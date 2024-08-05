#include <jni.h>
#include <string>
#include <thread>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <unistd.h>
#include "record_double_buffer.h"
#include "record_single_buffer.h"
#include "log/android_log.h"

#define DELETE_LOCAL_REF(env, obj)  if(obj!=NULL){env->DeleteLocalRef(obj);obj=NULL;}
static const char *const kClassPathName = "com/jiaquan/openslesrecord/MainActivity";

JavaVM *g_java_vm = NULL;
FILE *g_pcm_file = NULL;
RecordDoubleBuffer *g_record_double_buffer = NULL;
RecordSingleBuffer *g_record_single_buffer = NULL;
std::thread record_process_thread;
bool g_is_recording = false;
bool g_is_exit = false;
const int kBufferSize = 4096;
SLObjectItf slObjectEngine = NULL;
SLEngineItf engineItf = NULL;
SLObjectItf recordObj = NULL;
SLRecordItf recordItf = NULL;
SLAndroidSimpleBufferQueueItf recordBufferQueue = NULL;

/**
 * 音频录制回调
 */
void bqRecorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    if (!g_is_recording) {
        LOGE("stop recording,so do not enqueue buffer again.");
        return;
    }

    /**
     * 一次录制采样要求返回的数据buffer和大小，进行了固定设置:kBufferSize * 2字节
     * 这里使用的单缓冲区，采集回调的数据直接写入本地文件，可能会造成数据采集延时，但是可以保证数据不丢失
     * 且保证数据没有脏数据。
     * 如果使用双缓冲区，采集回调的数据写入缓冲区，然后在另外的线程中处理数据，这样可以保证数据采集的实时性，
     * 但是目前有问题，写入的本地数据有很大的杂音，可能是数据缓冲区的问题，后续再处理。
     */
//    if (g_pcm_file != NULL) {
//        fwrite((char *) g_record_single_buffer->GetRecordBuffer(), 1, kBufferSize * 2, g_pcm_file);
//        fflush(g_pcm_file);
//    }
//    (*recordBufferQueue)->Enqueue(recordBufferQueue, (char *) g_record_single_buffer->GetRecordBuffer(), kBufferSize * 2);

    short *write_buffer = g_record_double_buffer->GetWriteBuffer();
    if (write_buffer == NULL) {
        LOGE("bqRecorderCallback write_buffer is NULL");
        return;
    }
    /**
     * 一次录制采样要求返回的数据buffer和大小，进行了固定设置:kBufferSize * 2字节
     */
    SLresult result = (*recordBufferQueue)->Enqueue(recordBufferQueue, (char *) write_buffer, kBufferSize * 2);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("bqRecorderCallback Enqueue failed, result: %d", result);
    }
    g_record_double_buffer->CommitWriteBuffer(write_buffer);//提交已写入数据的缓冲区
}

void _ApplyFadeInEffect(short *buffer, int buffer_size, int fade_in_duration) {
    for (int i = 0; i < buffer_size && i < fade_in_duration; ++i) {
        float multiplier = static_cast<float>(i) / fade_in_duration;
        buffer[i] = static_cast<short>(buffer[i] * multiplier);
    }
}

void
_ApplyLowPassFilter(short *buffer, int buffer_size, int sample_rate, int cutoff_frequency = 3000) {
    // 简单的低通滤波器实现
    float rc = 1.0 / (cutoff_frequency * 2 * 3.14);
    float dt = 1.0 / sample_rate;
    float alpha = dt / (rc + dt);
    short previous = buffer[0];
    for (int i = 1; i < buffer_size; ++i) {
        buffer[i] = previous + alpha * (buffer[i] - previous);
        previous = buffer[i];
    }
}

/**
 * 处理录制数据并写入文件
 */
void _ProcessAudioData(RecordDoubleBuffer* recrod_buffer) {
    const int fade_in_duration = 44100; // 1 second fade-in at 44.1kHz
    bool fade_in_applied = false;

    while (!g_is_exit) {
        short *read_buffer = recrod_buffer->GetReadBuffer();
        if (read_buffer == NULL) {
            LOGE("_ProcessAudioData read_buffer is NULL");
            continue;
        }

        // Apply fade-in effect once at the start
//        if (!fade_in_applied) {
//            _ApplyFadeInEffect(read_buffer, kBufferSize, fade_in_duration);
//            fade_in_applied = true;
//        }
//
//        // 应用低通滤波器，cutoff_frequency 设置为 3000
//        _ApplyLowPassFilter(read_buffer, kBufferSize, 44100, 3000);
        if (g_pcm_file != NULL) {
            fwrite((char *) read_buffer, 1, kBufferSize * 2, g_pcm_file);
            fflush(g_pcm_file);
        }
        recrod_buffer->ReleaseReadBuffer(read_buffer);//释放已读取数据的缓冲区
    }
}

JNIEXPORT void JNICALL StartRecord(JNIEnv *env, jobject thiz, jstring path_jstr) {
    LOGI("StartRecord in, g_is_recording %d", g_is_recording);
    if (g_is_recording) {
        LOGE("StartRecord is running, can not call start again.");
        return;
    }
    g_is_recording = true;
    g_is_exit = false;
    g_record_double_buffer = new RecordDoubleBuffer(kBufferSize, 10, g_is_exit);
//    g_record_single_buffer = new RecordSingleBuffer(kBufferSize);
    const char *path = env->GetStringUTFChars(path_jstr, 0);
    if (g_pcm_file != NULL) {
        fclose(g_pcm_file);
        g_pcm_file = NULL;
    }
    g_pcm_file = fopen(path, "wb");
    if (g_pcm_file == NULL) {
        LOGE("open file %s failed.", path);
        return;
    }
    LOGI("StartRecord open file %s success.", path);
    // 创建OpenSL ES引擎
    slCreateEngine(&slObjectEngine, 0, NULL, 0, NULL, NULL);
    (*slObjectEngine)->Realize(slObjectEngine, SL_BOOLEAN_FALSE);
    (*slObjectEngine)->GetInterface(slObjectEngine, SL_IID_ENGINE, &engineItf);
    SLDataLocator_IODevice loc_dev = {SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT,
                                      SL_DEFAULTDEVICEID_AUDIOINPUT, NULL};//输入设备,表示音频录制
    SLDataSource audio_src = {&loc_dev, NULL};
    SLDataLocator_AndroidSimpleBufferQueue loc_bq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, 2, SL_SAMPLINGRATE_44_1,
                                   SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
                                   SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
                                   SL_BYTEORDER_LITTLEENDIAN};
    SLDataSink audio_snk = {&loc_bq, &format_pcm};
    const SLInterfaceID id[1] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};
    (*engineItf)->CreateAudioRecorder(engineItf, &recordObj, &audio_src, &audio_snk, 1, id,
                                      req);//创建录音器
    (*recordObj)->Realize(recordObj, SL_BOOLEAN_FALSE);//实现录音器
    (*recordObj)->GetInterface(recordObj, SL_IID_RECORD, &recordItf);//获取录音接口
    (*recordObj)->GetInterface(recordObj, SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
                               &recordBufferQueue);//获取缓冲队列接口
    (*recordBufferQueue)->RegisterCallback(recordBufferQueue, bqRecorderCallback, NULL);
    (*recordItf)->SetRecordState(recordItf, SL_RECORDSTATE_RECORDING);
    short *write_buffer = g_record_double_buffer->GetWriteBuffer();
    (*recordBufferQueue)->Enqueue(recordBufferQueue, (char *) write_buffer, kBufferSize * 2);
    g_record_double_buffer->CommitWriteBuffer(write_buffer);
//    (*recordBufferQueue)->Enqueue(recordBufferQueue, (char*)g_record_single_buffer->GetRecordBuffer(), kBufferSize * 2);
    env->ReleaseStringUTFChars(path_jstr, path);

    // 启动录制数据处理线程
    record_process_thread = std::thread(_ProcessAudioData, g_record_double_buffer);
    LOGI("StartRecord out");
}

JNIEXPORT void JNICALL StopRecord(JNIEnv *env, jobject thiz) {
    LOGI("StopRecord in, g_is_recording %d", g_is_recording);
    if (!g_is_recording) {
        LOGE("StopRecord is already stopped, can not call stop again.");
        return;
    }
    g_is_exit = true;
    (*recordItf)->SetRecordState(recordItf, SL_RECORDSTATE_STOPPED);
    g_record_double_buffer->NotifyAll();
    // 确保录制处理线程结束
    if (record_process_thread.joinable()) {
        record_process_thread.join();
    }
    (*recordObj)->Destroy(recordObj);//执行此语句会卡住，暂时屏蔽
    recordObj = NULL;
    recordItf = NULL;
    (*slObjectEngine)->Destroy(slObjectEngine);
    slObjectEngine = NULL;
    engineItf = NULL;
    delete g_record_double_buffer;
    g_record_double_buffer = NULL;
//    delete g_record_single_buffer;
//    g_record_single_buffer = NULL;
    if (g_pcm_file != NULL) {
        fclose(g_pcm_file);
        g_pcm_file = NULL;
    }
    g_is_recording = false;
    g_is_exit = false;
    LOGI("StopRecord out");
}

static JNINativeMethod gMethods[] = {
        {"_nativeStartRecord", "(Ljava/lang/String;)V", (void *) StartRecord},
        {"_nativeStopRecord",  "()V",                   (void *) StopRecord},
};

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    g_java_vm = vm;
    if (g_java_vm->GetEnv((void **) (&env), JNI_VERSION_1_6) != JNI_OK) {
        LOGE("JNI_OnLoad GetEnv failed!");
        return -1;
    }
    assert(env != NULL);
    jclass clazz = env->FindClass(kClassPathName);
    if (clazz == NULL) {
        LOGE("class not found. %s", kClassPathName);
        return -1;
    }
    if (env->RegisterNatives(clazz, gMethods, sizeof(gMethods) / sizeof(gMethods[0])) < 0) {
        LOGE("RegisterNatives failed!");
        return -1;
    }
    DELETE_LOCAL_REF(env, clazz);
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *vm, void *reserved) {
    LOGI("JNI_OnUnload in");
    JNIEnv *env;
    if (vm->GetEnv((void **) (&env), JNI_VERSION_1_6) != JNI_OK) {
        LOGE("JNI_OnUnload GetEnv failed!");
        return;
    }
    assert(env != NULL);
    if (g_java_vm != NULL) {
        g_java_vm = NULL;
    }
    LOGI("JNI_OnUnload out");
}