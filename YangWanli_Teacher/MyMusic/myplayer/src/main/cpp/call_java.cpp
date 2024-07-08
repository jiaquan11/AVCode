#include "call_java.h"

CallJava::CallJava(JavaVM *vm, JNIEnv *env, jobject obj) {
    m_java_vm_ = vm;
    m_jni_env_ = env;
    m_jobj_ = env->NewGlobalRef(obj);//创建全局引用,防止被回收,可以跨线程使用
    jclass clz = m_jni_env_->GetObjectClass(m_jobj_);
    if (clz == NULL) {
        if (LOG_DEBUG) {
            LOGE("CallJava get jclass wrong");
        }
        return;
    }
    m_jmid_load_ = env->GetMethodID(clz, "onCallLoad", "(Z)V");
    m_jmid_prepared_ = env->GetMethodID(clz, "onCallPrepared", "()V");
    m_jmid_time_info_ = env->GetMethodID(clz, "onCallTimeInfo", "(II)V");
    m_jmid_pcm_info_ = env->GetMethodID(clz, "onCallPcmInfo", "(III)V");
    m_jmid_pcm_data_ = env->GetMethodID(clz, "onCallPcmData", "([BI)V");
    m_jmid_volumedb_ = env->GetMethodID(clz, "onCallVolumeDB", "(I)V");
    m_jmid_is_support_mediacodec_ = env->GetMethodID(clz, "onCallIsSupportMediaCodec", "(Ljava/lang/String;)Z");
    m_jmid_init_mediacodec_ = env->GetMethodID(clz, "onCallInitMediaCodec", "(Ljava/lang/String;II)V");
    m_jmid_decode_vpacket_ = env->GetMethodID(clz, "onCallDecodeVPacket", "([BID)V");
    m_jmid_render_yuv_ = env->GetMethodID(clz, "onCallRenderYUV", "(II[B[B[B)V");
    m_jmid_complete_ = env->GetMethodID(clz, "onCallComplete", "()V");
    m_jmid_error_ = env->GetMethodID(clz, "onCallError", "(ILjava/lang/String;)V");
    m_jmid_pcm_to_aac_ = env->GetMethodID(clz, "onCallPcmToAAC", "([BI)V");
}

CallJava::~CallJava() {
    LOGI("CallJava::~CallJava in");
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

    if (m_jobj_ != NULL) {
        JNIEnv* env = nullptr;
        bool need_detach = false;
        if (m_java_vm_->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
            JavaVMAttachArgs args = {0};
            args.version = JNI_VERSION_1_6;
            if (m_java_vm_->AttachCurrentThread(&env, &args) == JNI_OK) {
                need_detach = true;
            } else {
                LOGE("CallJava::~CallJava: Failed to attach current thread");
                return;
            }
        } else {
            LOGI("CallJava::~CallJava GetEnv is OK");
        }
        env->DeleteGlobalRef(m_jobj_);
        m_jobj_ = NULL;
        if (need_detach) {
            m_java_vm_->DetachCurrentThread();
        }
    }
    m_java_vm_ = NULL;
    m_jni_env_ = NULL;
    LOGI("CallJava::~CallJava out");
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

void CallJava::OnCallTimeInfo(int type, int curr, int total) {
    if (type == MAIN_THREAD) {
        m_jni_env_->CallVoidMethod(m_jobj_, m_jmid_time_info_, curr, total);
    } else if (type == CHILD_THREAD) {
        JNIEnv *env;
        if (m_java_vm_->AttachCurrentThread(&env, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jnienv wrong");
                return;
            }
        }
        env->CallVoidMethod(m_jobj_, m_jmid_time_info_, curr, total);
        m_java_vm_->DetachCurrentThread();
    }
}

void CallJava::OnCallPcmInfo(int type, int samplerate, int bit, int channels) {
    if (type == MAIN_THREAD) {
        m_jni_env_->CallVoidMethod(m_jobj_, m_jmid_pcm_info_, samplerate, bit, channels);
    } else if (type == CHILD_THREAD) {
        JNIEnv *env;
        if (m_java_vm_->AttachCurrentThread(&env, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jnienv wrong");
                return;
            }
        }
        env->CallVoidMethod(m_jobj_, m_jmid_pcm_info_, samplerate, bit, channels);
        m_java_vm_->DetachCurrentThread();
    }
}

void CallJava::OnCallPcmData(int type, void *buffer, int size) {
    if (type == MAIN_THREAD) {
        jbyteArray jbyte_array = m_jni_env_->NewByteArray(size);
        m_jni_env_->SetByteArrayRegion(jbyte_array, 0, size, static_cast<const jbyte *>(buffer));
        m_jni_env_->CallVoidMethod(m_jobj_, m_jmid_pcm_data_, jbyte_array, size);
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
        env->CallVoidMethod(m_jobj_, m_jmid_pcm_data_, jbyte_array, size);
        env->DeleteLocalRef(jbyte_array);
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

bool CallJava::OnCallIsSupportMediaCodec(int type, const char *codec_tag) {
    bool is_support = false;
    if (type == MAIN_THREAD) {
        jstring codec_tag_jstr = m_jni_env_->NewStringUTF(codec_tag);
        is_support = m_jni_env_->CallBooleanMethod(m_jobj_, m_jmid_is_support_mediacodec_, codec_tag_jstr);
        m_jni_env_->DeleteLocalRef(codec_tag_jstr);
        return is_support;
    } else if (type == CHILD_THREAD) {
        JNIEnv *env;
        if (m_java_vm_->AttachCurrentThread(&env, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jnienv wrong");
                return is_support;
            }
        }
        jstring codec_tag_jstr = env->NewStringUTF(codec_tag);
        is_support = env->CallBooleanMethod(m_jobj_, m_jmid_is_support_mediacodec_, codec_tag_jstr);
        env->DeleteLocalRef(codec_tag_jstr);
        m_java_vm_->DetachCurrentThread();
        return is_support;
    }
}

void CallJava::OnCallInitMediaCodec(int type, const char *codec_tag, int width, int height) {
    if (type == MAIN_THREAD) {
        jstring codec_tag_jstr = m_jni_env_->NewStringUTF(codec_tag);
        m_jni_env_->CallVoidMethod(m_jobj_, m_jmid_init_mediacodec_, codec_tag_jstr, width, height);
        m_jni_env_->DeleteLocalRef(codec_tag_jstr);
    } else if (type == CHILD_THREAD) {
        JNIEnv *env;
        if (m_java_vm_->AttachCurrentThread(&env, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jnienv wrong");
                return;
            }
        }
        jstring codec_tag_jstr = env->NewStringUTF(codec_tag);
        env->CallVoidMethod(m_jobj_, m_jmid_init_mediacodec_, codec_tag_jstr, width, height);
        env->DeleteLocalRef(codec_tag_jstr);
        m_java_vm_->DetachCurrentThread();
    }
}

void CallJava::OnCallDecodeVPacket(int type, uint8_t *packetdata, int data_size, double pts_secds) {
    if (type == MAIN_THREAD) {
        jbyteArray data = m_jni_env_->NewByteArray(data_size);
        m_jni_env_->SetByteArrayRegion(data, 0, data_size, reinterpret_cast<const jbyte *>(packetdata));
        m_jni_env_->CallVoidMethod(m_jobj_, m_jmid_decode_vpacket_, data, data_size, pts_secds);
        m_jni_env_->DeleteLocalRef(data);
    } else if (type == CHILD_THREAD) {
        JNIEnv *env;
        if (m_java_vm_->AttachCurrentThread(&env, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jnienv wrong");
                return;
            }
        }
        jbyteArray data = env->NewByteArray(data_size);
        env->SetByteArrayRegion(data, 0, data_size, reinterpret_cast<const jbyte *>(packetdata));
        env->CallVoidMethod(m_jobj_, m_jmid_decode_vpacket_, data, data_size, pts_secds);
        env->DeleteLocalRef(data);
        m_java_vm_->DetachCurrentThread();
    }
}

void CallJava::OnCallRenderYUV(int type, int width, int height, int linesize, uint8_t *y_data, uint8_t *u_data, uint8_t *v_data) {
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

        _CutAndCopyYuv(m_data_,y_data,u_data,v_data, width, height, linesize);

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

void CallJava::OnCallPcmToAAC(int type, void *buffer, int size) {
    if (type == MAIN_THREAD) {
        jbyteArray jbuffer = m_jni_env_->NewByteArray(size);
        m_jni_env_->SetByteArrayRegion(jbuffer, 0, size, static_cast<const jbyte *>(buffer));
        m_jni_env_->CallVoidMethod(m_jobj_, m_jmid_pcm_to_aac_, jbuffer, size);
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
        env->CallVoidMethod(m_jobj_, m_jmid_pcm_to_aac_, jbuffer, size);
        env->DeleteLocalRef(jbuffer);
        m_java_vm_->DetachCurrentThread();
    }
}

void CallJava::_CutAndCopyYuv(uint8_t* data[], uint8_t *srcy, uint8_t *srcu, uint8_t *srcv, int width, int height, int linesize) {
    unsigned char *y_dest = data[0];
    unsigned char *u_dest = data[1];
    unsigned char *v_dest = data[2];
    unsigned char *y_src = srcy;
    if ((y_dest != NULL) && (u_dest != NULL) && (v_dest != NULL)) {
        for (int i = 0; i < height; i++) {
            memcpy(y_dest, y_src, width);
            y_dest += width;
            y_src += linesize;
        }

        int linesize_uv = linesize / 2;
        int width_uv = width / 2;
        int height_uv = height / 2;
        unsigned char *u_src = srcu;
        for (int i = 0; i < height_uv; ++i) {
            memcpy(u_dest, u_src, width_uv);
            u_dest += width_uv;
            u_src += linesize_uv;
        }

        unsigned char *v_src = srcv;
        for (int i = 0; i < height_uv; ++i) {
            memcpy(v_dest, v_src, width_uv);
            v_dest += width_uv;
            v_src += linesize_uv;
        }
    }
}
