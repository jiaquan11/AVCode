#include "call_java.h"

CallJava::CallJava(JavaVM *vm, JNIEnv *env, jobject obj) {
    m_java_vm_ = vm;
    m_jni_env_ = env;
    m_jobj_ = env->NewGlobalRef(obj);
    jclass clz = m_jni_env_->GetObjectClass(m_jobj_);
    if (!clz) {
        if (LOG_DEBUG) {
            LOGE("CallJava get jclass wrong");
        }
        return;
    }

    //获取Java类的WLPlayer方法ID,用于底层回调Java方法
    m_jmid_load_ = env->GetMethodID(clz, "onCallLoad", "(Z)V");
    m_jmid_prepared_ = env->GetMethodID(clz, "onCallPrepared", "()V");
    m_jmid_timeinfo_ = env->GetMethodID(clz, "onCallTimeInfo", "(II)V");
    m_jmid_complete_ = env->GetMethodID(clz, "onCallComplete", "()V");
    m_jmid_error_ = env->GetMethodID(clz, "onCallError", "(ILjava/lang/String;)V");
    m_jmid_volumedb_ = env->GetMethodID(clz, "onCallVolumeDB", "(I)V");
    m_jmid_pcminfo_ = env->GetMethodID(clz, "onCallPcmInfo", "([BI)V");
    m_jmid_pcmrate_ = env->GetMethodID(clz, "onCallPcmRate", "(III)V");
    m_jmid_pcmtoaac_ = env->GetMethodID(clz, "encodePcmToAAC", "([BI)V");
    m_jmid_support_video_ = env->GetMethodID(clz, "onCallIsSupportMediaCodec", "(Ljava/lang/String;)Z");
    m_jmid_init_mediacodec_ = env->GetMethodID(clz, "onCallInitMediaCodec", "(Ljava/lang/String;II[B)V");
    m_jmid_render_yuv_ = env->GetMethodID(clz, "onCallRenderYUV", "(II[B[B[B)V");
    m_jmid_decode_vpacket_ = env->GetMethodID(clz, "onCallDecodeVPacket", "(I[B)V");
}

CallJava::~CallJava() {
    if (m_has_allocate_) {
        if (m_data_[0] != NULL) {
            free(m_data_[0]);
            m_data_[0] = NULL;
        }
        if (m_data_[1] != NULL) {
            free(m_data_[1]);
            m_data_[1] = NULL;
        }
        if (m_data_[2] != NULL) {
            free(m_data_[2]);
            m_data_[2] = NULL;
        }
        m_has_allocate_ = false;
    }
}

void CallJava::OnCallPrepared(int type) {
    if (type == MAIN_THREAD) {
        m_jni_env_->CallVoidMethod(m_jobj_, m_jmid_prepared_);
    } else if (type == CHILD_THREAD) {
        JNIEnv *env;
        if (m_java_vm_->AttachCurrentThread(&env, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jnienv wrong");
                return;
            }
        }
        env->CallVoidMethod(m_jobj_, m_jmid_prepared_);
        m_java_vm_->DetachCurrentThread();
    }
}

void CallJava::OnCallLoad(int type, bool load) {
    if (type == MAIN_THREAD) {
        m_jni_env_->CallVoidMethod(m_jobj_, m_jmid_load_, load);
    } else if (type == CHILD_THREAD) {
        JNIEnv *env;
        if (m_java_vm_->AttachCurrentThread(&env, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jnienv wrong");
                return;
            }
        }
        env->CallVoidMethod(m_jobj_, m_jmid_load_, load);
        m_java_vm_->DetachCurrentThread();
    }
}

void CallJava::OnCallTimeInfo(int type, int curr, int total) {
    if (type == MAIN_THREAD) {
        m_jni_env_->CallVoidMethod(m_jobj_, m_jmid_timeinfo_, curr, total);
    } else if (type == CHILD_THREAD) {
        JNIEnv *env;
        if (m_java_vm_->AttachCurrentThread(&env, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jnienv wrong");
                return;
            }
        }
        env->CallVoidMethod(m_jobj_, m_jmid_timeinfo_, curr, total);
        m_java_vm_->DetachCurrentThread();
    }
}

void CallJava::OnCallComplete(int type) {
    if (type == MAIN_THREAD) {
        m_jni_env_->CallVoidMethod(m_jobj_, m_jmid_complete_);
    } else if (type == CHILD_THREAD) {
        JNIEnv *env;
        if (m_java_vm_->AttachCurrentThread(&env, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jnienv wrong");
                return;
            }
        }
        env->CallVoidMethod(m_jobj_, m_jmid_complete_);
        m_java_vm_->DetachCurrentThread();
    }
}

void CallJava::OnCallError(int type, int code, char *msg) {
    if (type == MAIN_THREAD) {
        jstring jmsg = m_jni_env_->NewStringUTF(msg);
        m_jni_env_->CallVoidMethod(m_jobj_, m_jmid_error_, code, jmsg);
        m_jni_env_->DeleteLocalRef(jmsg);
    } else if (type == CHILD_THREAD) {
        JNIEnv *env;
        if (m_java_vm_->AttachCurrentThread(&env, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jnienv wrong");
                return;
            }
        }
        jstring jmsg = env->NewStringUTF(msg);
        env->CallVoidMethod(m_jobj_, m_jmid_error_, code, jmsg);
        env->DeleteLocalRef(jmsg);
        m_java_vm_->DetachCurrentThread();
    }
}

void CallJava::OnCallVolumeDB(int type, int db) {
    if (type == MAIN_THREAD) {
        m_jni_env_->CallVoidMethod(m_jobj_, m_jmid_volumedb_, db);
    } else if (type == CHILD_THREAD) {
        JNIEnv *env;
        if (m_java_vm_->AttachCurrentThread(&env, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jnienv wrong");
                return;
            }
        }
        env->CallVoidMethod(m_jobj_, m_jmid_volumedb_, db);
        m_java_vm_->DetachCurrentThread();
    }
}

void CallJava::OnCallPcmInfo(int type, void *buffer, int size) {
    if (type == MAIN_THREAD) {
        jbyteArray jbyte_array = m_jni_env_->NewByteArray(size);
        m_jni_env_->SetByteArrayRegion(jbyte_array, 0, size, static_cast<const jbyte *>(buffer));
        m_jni_env_->CallVoidMethod(m_jobj_, m_jmid_pcminfo_, jbyte_array, size);
        m_jni_env_->DeleteLocalRef(jbyte_array);
    } else if (type == CHILD_THREAD) {
        JNIEnv *env;
        if (m_java_vm_->AttachCurrentThread(&env, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jnienv wrong");
                return;
            }
        }

        jbyteArray jbyte_array = env->NewByteArray(size);
        env->SetByteArrayRegion(jbyte_array, 0, size, static_cast<const jbyte *>(buffer));
        env->CallVoidMethod(m_jobj_, m_jmid_pcminfo_, jbyte_array, size);
        env->DeleteLocalRef(jbyte_array);
        m_java_vm_->DetachCurrentThread();
    }
}

void CallJava::OnCallPcmRate(int type, int samplerate, int bit, int channels) {
    if (type == MAIN_THREAD) {
        m_jni_env_->CallVoidMethod(m_jobj_, m_jmid_pcmrate_, samplerate, bit, channels);
    } else if (type == CHILD_THREAD) {
        JNIEnv *env;
        if (m_java_vm_->AttachCurrentThread(&env, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jnienv wrong");
                return;
            }
        }
        env->CallVoidMethod(m_jobj_, m_jmid_pcmrate_, samplerate, bit, channels);
        m_java_vm_->DetachCurrentThread();
    }
}

void CallJava::OnCallPcmToAAC(int type, void *buffer, int size) {
    if (type == MAIN_THREAD) {
        jbyteArray jbuffer = m_jni_env_->NewByteArray(size);
        m_jni_env_->SetByteArrayRegion(jbuffer, 0, size, static_cast<const jbyte *>(buffer));

        m_jni_env_->CallVoidMethod(m_jobj_, m_jmid_pcmtoaac_, jbuffer, size);
        m_jni_env_->DeleteLocalRef(jbuffer);
    } else if (type == CHILD_THREAD) {
        JNIEnv *env;
        if (m_java_vm_->AttachCurrentThread(&env, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jnienv wrong");
                return;
            }
        }

        jbyteArray jbuffer = env->NewByteArray(size);
        env->SetByteArrayRegion(jbuffer, 0, size, static_cast<const jbyte *>(buffer));
        env->CallVoidMethod(m_jobj_, m_jmid_pcmtoaac_, jbuffer, size);
        env->DeleteLocalRef(jbuffer);
        m_java_vm_->DetachCurrentThread();
    }
}

bool CallJava::OnCallIsSupportVideo(int type, const char *ffcodecname) {
    bool is_support = false;
    if (type == MAIN_THREAD) {
        jstring type = m_jni_env_->NewStringUTF(ffcodecname);
        is_support = m_jni_env_->CallBooleanMethod(m_jobj_, m_jmid_support_video_, type);
        m_jni_env_->DeleteLocalRef(type);
        return is_support;
    } else if (type == CHILD_THREAD) {
        JNIEnv *env;
        if (m_java_vm_->AttachCurrentThread(&env, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jnienv wrong");
                return is_support;
            }
        }

        jstring type = env->NewStringUTF(ffcodecname);
        is_support = env->CallBooleanMethod(m_jobj_, m_jmid_support_video_, type);
        env->DeleteLocalRef(type);
        m_java_vm_->DetachCurrentThread();
        return is_support;
    }
}

void CallJava::OnCallInitMediaCodec(int type, const char *mime, int width, int height, int csd_size, uint8_t *csd) {
    if (type == MAIN_THREAD) {
        jstring typejstr = m_jni_env_->NewStringUTF(mime);
        jbyteArray csdArray = m_jni_env_->NewByteArray(csd_size);
        m_jni_env_->SetByteArrayRegion(csdArray, 0, csd_size, reinterpret_cast<const jbyte *>(csd));

        m_jni_env_->CallVoidMethod(m_jobj_, m_jmid_init_mediacodec_, typejstr, width, height, csdArray);
        m_jni_env_->DeleteLocalRef(csdArray);
        m_jni_env_->DeleteLocalRef(typejstr);
    } else if (type == CHILD_THREAD) {
        JNIEnv *env;
        if (m_java_vm_->AttachCurrentThread(&env, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jnienv wrong");
                return;
            }
        }

        jstring typejstr = env->NewStringUTF(mime);
        jbyteArray csdArray = env->NewByteArray(csd_size);
        env->SetByteArrayRegion(csdArray, 0, csd_size, reinterpret_cast<const jbyte *>(csd));
        env->CallVoidMethod(m_jobj_, m_jmid_init_mediacodec_, typejstr, width, height, csdArray);
        env->DeleteLocalRef(csdArray);
        env->DeleteLocalRef(typejstr);
        m_java_vm_->DetachCurrentThread();
    }
}

void CallJava::OnCallRenderYUV(int type, int width, int linesize, int height, uint8_t *fy, uint8_t *fu, uint8_t *fv) {
    LOGI("onCallRenderYUV type:%d, width:%d, linesize:%d, height:%d", type, width, linesize, height);
    uint8_t* pFY = fy;
    uint8_t* pFU = fu;
    uint8_t* pFV = fv;
    if (width != linesize) {//对于linesize多余的部分需要进行裁剪
        int ysize = width * height;
        int uvsize = ysize / 4;
        if (!m_has_allocate_) {
            m_data_[0] = static_cast<uint8_t *>(malloc(ysize * sizeof(uint8_t)));
            m_data_[1] = static_cast<uint8_t *>(malloc(uvsize * sizeof(uint8_t)));
            m_data_[2] = static_cast<uint8_t *>(malloc(uvsize * sizeof(uint8_t)));
            m_has_allocate_ = true;
        }

        _CutAndCopyYuv(m_data_, fy, fu, fv, linesize, width, height);

        pFY = m_data_[0];
        pFU = m_data_[1];
        pFV = m_data_[2];
    }

    if (type == MAIN_THREAD) {
        jbyteArray y = m_jni_env_->NewByteArray(width * height);
        m_jni_env_->SetByteArrayRegion(y, 0, width * height, reinterpret_cast<const jbyte *>(pFY));

        jbyteArray u = m_jni_env_->NewByteArray(width * height / 4);
        m_jni_env_->SetByteArrayRegion(u, 0, width * height / 4, reinterpret_cast<const jbyte *>(pFU));

        jbyteArray v = m_jni_env_->NewByteArray(width * height / 4);
        m_jni_env_->SetByteArrayRegion(v, 0, width * height / 4, reinterpret_cast<const jbyte *>(pFV));

        m_jni_env_->CallVoidMethod(m_jobj_, m_jmid_render_yuv_, width, height, y, u, v);

        m_jni_env_->DeleteLocalRef(y);
        m_jni_env_->DeleteLocalRef(u);
        m_jni_env_->DeleteLocalRef(v);
    } else if (type == CHILD_THREAD) {
        JNIEnv *env;
        if (m_java_vm_->AttachCurrentThread(&env, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jnienv wrong");
                return;
            }
        }

        jbyteArray y = env->NewByteArray(width * height);
        env->SetByteArrayRegion(y, 0, width * height, reinterpret_cast<const jbyte *>(pFY));

        jbyteArray u = env->NewByteArray(width * height / 4);
        env->SetByteArrayRegion(u, 0, width * height / 4, reinterpret_cast<const jbyte *>(pFU));

        jbyteArray v = env->NewByteArray(width * height / 4);
        env->SetByteArrayRegion(v, 0, width * height / 4, reinterpret_cast<const jbyte *>(pFV));

        env->CallVoidMethod(m_jobj_, m_jmid_render_yuv_, width, height, y, u, v);

        env->DeleteLocalRef(y);
        env->DeleteLocalRef(u);
        env->DeleteLocalRef(v);
        m_java_vm_->DetachCurrentThread();
    }
}

void CallJava::OnCallDecodeVPacket(int type, int datasize, uint8_t *packetdata) {
    if (type == MAIN_THREAD) {
        jbyteArray data = m_jni_env_->NewByteArray(datasize);
        m_jni_env_->SetByteArrayRegion(data, 0, datasize, reinterpret_cast<const jbyte *>(packetdata));
        m_jni_env_->CallVoidMethod(m_jobj_, m_jmid_decode_vpacket_, datasize, data);
        m_jni_env_->DeleteLocalRef(data);
    } else if (type == CHILD_THREAD) {
        JNIEnv *env;
        if (m_java_vm_->AttachCurrentThread(&env, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jnienv wrong");
                return;
            }
        }

        jbyteArray data = env->NewByteArray(datasize);
        env->SetByteArrayRegion(data, 0, datasize, reinterpret_cast<const jbyte *>(packetdata));
        env->CallVoidMethod(m_jobj_, m_jmid_decode_vpacket_, datasize, data);
        env->DeleteLocalRef(data);
        m_java_vm_->DetachCurrentThread();
    }
}

void CallJava::_CutAndCopyYuv(uint8_t* data[], uint8_t *srcfy, uint8_t *srcfu, uint8_t *srcfv, int linesize, int width, int height) {
    unsigned char *pDestY = data[0];
    unsigned char *pDestU = data[1];
    unsigned char *pDestV = data[2];
    unsigned char *pDataY = srcfy;
    if ((pDestY != NULL) && (pDestU != NULL) && (pDestV != NULL)) {
        for (int i = 0; i < height; i++) {
            memcpy(pDestY, pDataY, width);
            pDestY += width;
            pDataY += linesize;
        }

        int lineSizeUV = linesize / 2;
        int widthUV = width / 2;
        int heightUV = height / 2;
        unsigned char *pDataU = srcfu;
        for (int i = 0; i < heightUV; ++i) {
            memcpy(pDestU, pDataU, widthUV);
            pDestU += widthUV;
            pDataU += lineSizeUV;
        }

        unsigned char *pDataV = srcfv;
        for (int i = 0; i < heightUV; ++i) {
            memcpy(pDestV, pDataV, widthUV);
            pDestV += widthUV;
            pDataV += lineSizeUV;
        }
    }
}
