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
    if (m_jobj_ != NULL) {
        JNIEnv* env = nullptr;
        bool needDetach = false;
        if (m_java_vm_->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
            if (m_java_vm_->AttachCurrentThread(&env, nullptr) == JNI_OK) {
                needDetach = true;
            } else {
                LOGE("CallJava::~CallJava: Failed to attach current thread");
                return;
            }
        }
        env->DeleteGlobalRef(m_jobj_);
        m_jobj_ = NULL;
        if (needDetach) {
            m_java_vm_->DetachCurrentThread();
        }
    }
    m_jni_env_ = NULL;
    m_java_vm_ = NULL;

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
    LOGI("CallJava::~CallJava 22");
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

bool CallJava::OnCallIsSupportVideo(int type, const char *codec_tag) {
    bool is_support = false;
    if (type == MAIN_THREAD) {
        jstring type = m_jni_env_->NewStringUTF(codec_tag);
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

        jstring type = env->NewStringUTF(codec_tag);
        is_support = env->CallBooleanMethod(m_jobj_, m_jmid_support_video_, type);
        env->DeleteLocalRef(type);
        m_java_vm_->DetachCurrentThread();
        return is_support;
    }
}

void CallJava::OnCallInitMediaCodec(int type, const char *mime, int width, int height, int csd_size, uint8_t *csd) {
    if (type == MAIN_THREAD) {
        jstring typejstr = m_jni_env_->NewStringUTF(mime);
        jbyteArray csd_array = m_jni_env_->NewByteArray(csd_size);
        m_jni_env_->SetByteArrayRegion(csd_array, 0, csd_size, reinterpret_cast<const jbyte *>(csd));

        m_jni_env_->CallVoidMethod(m_jobj_, m_jmid_init_mediacodec_, typejstr, width, height, csd_array);
        m_jni_env_->DeleteLocalRef(csd_array);
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
        jbyteArray csd_array = env->NewByteArray(csd_size);
        env->SetByteArrayRegion(csd_array, 0, csd_size, reinterpret_cast<const jbyte *>(csd));
        env->CallVoidMethod(m_jobj_, m_jmid_init_mediacodec_, typejstr, width, height, csd_array);
        env->DeleteLocalRef(csd_array);
        env->DeleteLocalRef(typejstr);
        m_java_vm_->DetachCurrentThread();
    }
}

void CallJava::OnCallRenderYUV(int type, int width, int linesize, int height, uint8_t *y_data, uint8_t *u_data, uint8_t *v_data) {
//    LOGI("onCallRenderYUV type:%d, width:%d, linesize:%d, height:%d", type, width, linesize, height);
    uint8_t* py = y_data;
    uint8_t* pu = u_data;
    uint8_t* pv = v_data;
    if (width != linesize) {//对于linesize多余的部分需要进行裁剪
        int ysize = width * height;
        int uvsize = ysize / 4;
        if (!m_has_allocate_) {
            m_data_[0] = static_cast<uint8_t *>(malloc(ysize * sizeof(uint8_t)));
            m_data_[1] = static_cast<uint8_t *>(malloc(uvsize * sizeof(uint8_t)));
            m_data_[2] = static_cast<uint8_t *>(malloc(uvsize * sizeof(uint8_t)));
            m_has_allocate_ = true;
        }

        _CutAndCopyYuv(m_data_, y_data, u_data, v_data, linesize, width, height);

        py = m_data_[0];
        pu = m_data_[1];
        pv = m_data_[2];
    }

    if (type == MAIN_THREAD) {
        jbyteArray y_array = m_jni_env_->NewByteArray(width * height);
        m_jni_env_->SetByteArrayRegion(y_array, 0, width * height, reinterpret_cast<const jbyte *>(py));

        jbyteArray u_array = m_jni_env_->NewByteArray(width * height / 4);
        m_jni_env_->SetByteArrayRegion(u_array, 0, width * height / 4, reinterpret_cast<const jbyte *>(pu));

        jbyteArray v_array = m_jni_env_->NewByteArray(width * height / 4);
        m_jni_env_->SetByteArrayRegion(v_array, 0, width * height / 4, reinterpret_cast<const jbyte *>(pv));

        m_jni_env_->CallVoidMethod(m_jobj_, m_jmid_render_yuv_, width, height, y_array, u_array, v_array);

        m_jni_env_->DeleteLocalRef(y_array);
        m_jni_env_->DeleteLocalRef(u_array);
        m_jni_env_->DeleteLocalRef(v_array);
    } else if (type == CHILD_THREAD) {
        JNIEnv *env;
        if (m_java_vm_->AttachCurrentThread(&env, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jnienv wrong");
                return;
            }
        }

        jbyteArray y_array = env->NewByteArray(width * height);
        env->SetByteArrayRegion(y_array, 0, width * height, reinterpret_cast<const jbyte *>(py));
        jbyteArray u_array = env->NewByteArray(width * height / 4);
        env->SetByteArrayRegion(u_array, 0, width * height / 4, reinterpret_cast<const jbyte *>(pu));
        jbyteArray v_array = env->NewByteArray(width * height / 4);
        env->SetByteArrayRegion(v_array, 0, width * height / 4, reinterpret_cast<const jbyte *>(pv));

        env->CallVoidMethod(m_jobj_, m_jmid_render_yuv_, width, height, y_array, u_array, v_array);

        env->DeleteLocalRef(y_array);
        env->DeleteLocalRef(u_array);
        env->DeleteLocalRef(v_array);
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
    unsigned char *y_dest = data[0];
    unsigned char *u_dest = data[1];
    unsigned char *v_dest = data[2];
    unsigned char *y_src = srcfy;
    if ((y_dest != NULL) && (u_dest != NULL) && (v_dest != NULL)) {
        for (int i = 0; i < height; i++) {
            memcpy(y_dest, y_src, width);
            y_dest += width;
            y_src += linesize;
        }

        int linesize_uv = linesize / 2;
        int width_uv = width / 2;
        int height_uv = height / 2;
        unsigned char *u_src = srcfu;
        for (int i = 0; i < height_uv; ++i) {
            memcpy(u_dest, u_src, width_uv);
            u_dest += width_uv;
            u_src += linesize_uv;
        }

        unsigned char *v_src = srcfv;
        for (int i = 0; i < height_uv; ++i) {
            memcpy(v_dest, v_src, width_uv);
            v_dest += width_uv;
            v_src += linesize_uv;
        }
    }
}
