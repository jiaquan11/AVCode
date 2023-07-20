#include "CallJava.h"

CallJava::CallJava(JavaVM *vm, JNIEnv *env, jobject obj) {
    javaVm = vm;
    jniEnv = env;
    jobj = env->NewGlobalRef(obj);
    jclass clz = jniEnv->GetObjectClass(jobj);
    if (!clz) {
        if (LOG_DEBUG) {
            LOGE("CallJava get jclass wrong");
        }
        return;
    }

    //获取Java类的WLPlayer方法ID,用于底层回调Java方法
    jmid_prepared = env->GetMethodID(clz, "onCallPrepared", "()V");
    jmid_load = env->GetMethodID(clz, "onCallLoad", "(Z)V");
    jmid_timeinfo = env->GetMethodID(clz, "onCallTimeInfo", "(II)V");
    jmid_complete = env->GetMethodID(clz, "onCallComplete", "()V");
    jmid_error = env->GetMethodID(clz, "onCallError", "(ILjava/lang/String;)V");
    jmid_volumeDB = env->GetMethodID(clz, "onCallVolumeDB", "(I)V");
    jmid_pcminfo = env->GetMethodID(clz, "onCallPcmInfo", "([BI)V");
    jmid_pcmrate = env->GetMethodID(clz, "onCallPcmRate", "(III)V");
    jmid_pcmtoaac = env->GetMethodID(clz, "encodePcmToAAC", "([BI)V");
    jmid_supportvideo = env->GetMethodID(clz, "onCallIsSupportMediaCodec", "(Ljava/lang/String;)Z");
    jmid_initmediacodec = env->GetMethodID(clz, "onCallinitMediaCodec", "(Ljava/lang/String;II[B)V");
    jmid_renderyuv = env->GetMethodID(clz, "onCallRenderYUV", "(II[B[B[B)V");
    jmid_decodeVPacket = env->GetMethodID(clz, "onCallDecodeVPacket", "(I[B)V");
}

CallJava::~CallJava() {
    if (bHasAllocate) {
        if (pData[0] != NULL) {
            free(pData[0]);
            pData[0] = NULL;
        }
        if (pData[1] != NULL) {
            free(pData[1]);
            pData[1] = NULL;
        }
        if (pData[2] != NULL) {
            free(pData[2]);
            pData[2] = NULL;
        }
        bHasAllocate = false;
    }
}

void CallJava::onCallPrepared(int type) {
    if (type == MAIN_THREAD) {
        jniEnv->CallVoidMethod(jobj, jmid_prepared);
    } else if (type == CHILD_THREAD) {
        JNIEnv *env;
        if (javaVm->AttachCurrentThread(&env, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jnienv wrong");
                return;
            }
        }
        env->CallVoidMethod(jobj, jmid_prepared);
        javaVm->DetachCurrentThread();
    }
}

void CallJava::onCallLoad(int type, bool load) {
    if (type == MAIN_THREAD) {
        jniEnv->CallVoidMethod(jobj, jmid_load, load);
    } else if (type == CHILD_THREAD) {
        JNIEnv *env;
        if (javaVm->AttachCurrentThread(&env, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jnienv wrong");
                return;
            }
        }
        env->CallVoidMethod(jobj, jmid_load, load);
        javaVm->DetachCurrentThread();
    }
}

void CallJava::onCallTimeInfo(int type, int curr, int total) {
    if (type == MAIN_THREAD) {
        jniEnv->CallVoidMethod(jobj, jmid_timeinfo, curr, total);
    } else if (type == CHILD_THREAD) {
        JNIEnv *env;
        if (javaVm->AttachCurrentThread(&env, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jnienv wrong");
                return;
            }
        }
        env->CallVoidMethod(jobj, jmid_timeinfo, curr, total);
        javaVm->DetachCurrentThread();
    }
}

void CallJava::onCallComplete(int type) {
    if (type == MAIN_THREAD) {
        jniEnv->CallVoidMethod(jobj, jmid_complete);
    } else if (type == CHILD_THREAD) {
        JNIEnv *env;
        if (javaVm->AttachCurrentThread(&env, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jnienv wrong");
                return;
            }
        }
        env->CallVoidMethod(jobj, jmid_complete);
        javaVm->DetachCurrentThread();
    }
}

void CallJava::onCallError(int type, int code, char *msg) {
    if (type == MAIN_THREAD) {
        jstring jmsg = jniEnv->NewStringUTF(msg);
        jniEnv->CallVoidMethod(jobj, jmid_error, code, jmsg);
        jniEnv->DeleteLocalRef(jmsg);
    } else if (type == CHILD_THREAD) {
        JNIEnv *env;
        if (javaVm->AttachCurrentThread(&env, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jnienv wrong");
                return;
            }
        }
        jstring jmsg = env->NewStringUTF(msg);
        env->CallVoidMethod(jobj, jmid_error, code, jmsg);
        env->DeleteLocalRef(jmsg);
        javaVm->DetachCurrentThread();
    }
}

void CallJava::onCallVolumeDB(int type, int db) {
    if (type == MAIN_THREAD) {
        jniEnv->CallVoidMethod(jobj, jmid_volumeDB, db);
    } else if (type == CHILD_THREAD) {
        JNIEnv *env;
        if (javaVm->AttachCurrentThread(&env, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jnienv wrong");
                return;
            }
        }
        env->CallVoidMethod(jobj, jmid_volumeDB, db);
        javaVm->DetachCurrentThread();
    }
}

void CallJava::onCallPcmInfo(int type, void *buffer, int size) {
    if (type == MAIN_THREAD) {
        jbyteArray jbuffer = jniEnv->NewByteArray(size);
        jniEnv->SetByteArrayRegion(jbuffer, 0, size, static_cast<const jbyte *>(buffer));
        jniEnv->CallVoidMethod(jobj, jmid_pcminfo, jbuffer, size);
        jniEnv->DeleteLocalRef(jbuffer);
    } else if (type == CHILD_THREAD) {
        JNIEnv *env;
        if (javaVm->AttachCurrentThread(&env, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jnienv wrong");
                return;
            }
        }

        jbyteArray jbuffer = env->NewByteArray(size);
        env->SetByteArrayRegion(jbuffer, 0, size, static_cast<const jbyte *>(buffer));
        env->CallVoidMethod(jobj, jmid_pcminfo, jbuffer, size);
        env->DeleteLocalRef(jbuffer);
        javaVm->DetachCurrentThread();
    }
}

void CallJava::onCallPcmRate(int type, int samplerate, int bit, int channels) {
    if (type == MAIN_THREAD) {
        jniEnv->CallVoidMethod(jobj, jmid_pcmrate, samplerate, bit, channels);
    } else if (type == CHILD_THREAD) {
        JNIEnv *env;
        if (javaVm->AttachCurrentThread(&env, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jnienv wrong");
                return;
            }
        }
        env->CallVoidMethod(jobj, jmid_pcmrate, samplerate, bit, channels);
        javaVm->DetachCurrentThread();
    }
}

void CallJava::onCallPcmToAAC(int type, void *buffer, int size) {
    if (type == MAIN_THREAD) {
        jbyteArray jbuffer = jniEnv->NewByteArray(size);
        jniEnv->SetByteArrayRegion(jbuffer, 0, size, static_cast<const jbyte *>(buffer));

        jniEnv->CallVoidMethod(jobj, jmid_pcmtoaac, jbuffer, size);
        jniEnv->DeleteLocalRef(jbuffer);
    } else if (type == CHILD_THREAD) {
        JNIEnv *env;
        if (javaVm->AttachCurrentThread(&env, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jnienv wrong");
                return;
            }
        }

        jbyteArray jbuffer = env->NewByteArray(size);
        env->SetByteArrayRegion(jbuffer, 0, size, static_cast<const jbyte *>(buffer));
        env->CallVoidMethod(jobj, jmid_pcmtoaac, jbuffer, size);
        env->DeleteLocalRef(jbuffer);
        javaVm->DetachCurrentThread();
    }
}

bool CallJava::onCallIsSupportVideo(int type, const char *ffcodecname) {
    bool support = false;
    if (type == MAIN_THREAD) {
        jstring type = jniEnv->NewStringUTF(ffcodecname);
        support = jniEnv->CallBooleanMethod(jobj, jmid_supportvideo, type);
        jniEnv->DeleteLocalRef(type);
        return support;
    } else if (type == CHILD_THREAD) {
        JNIEnv *env;
        if (javaVm->AttachCurrentThread(&env, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jnienv wrong");
                return support;
            }
        }

        jstring type = env->NewStringUTF(ffcodecname);
        support = env->CallBooleanMethod(jobj, jmid_supportvideo, type);
        env->DeleteLocalRef(type);
        javaVm->DetachCurrentThread();
        return support;
    }
}

void CallJava::onCallinitMediaCodec(int type, const char *mime, int width, int height, int csd_size, uint8_t *csd) {
    if (type == MAIN_THREAD) {
        jstring typejstr = jniEnv->NewStringUTF(mime);
        jbyteArray csdArray = jniEnv->NewByteArray(csd_size);
        jniEnv->SetByteArrayRegion(csdArray, 0, csd_size, reinterpret_cast<const jbyte *>(csd));

        jniEnv->CallVoidMethod(jobj, jmid_initmediacodec, typejstr, width, height, csdArray);
        jniEnv->DeleteLocalRef(csdArray);
        jniEnv->DeleteLocalRef(typejstr);
    } else if (type == CHILD_THREAD) {
        JNIEnv *env;
        if (javaVm->AttachCurrentThread(&env, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jnienv wrong");
                return;
            }
        }

        jstring typejstr = env->NewStringUTF(mime);
        jbyteArray csdArray = env->NewByteArray(csd_size);
        env->SetByteArrayRegion(csdArray, 0, csd_size, reinterpret_cast<const jbyte *>(csd));
        env->CallVoidMethod(jobj, jmid_initmediacodec, typejstr, width, height, csdArray);
        env->DeleteLocalRef(csdArray);
        env->DeleteLocalRef(typejstr);
        javaVm->DetachCurrentThread();
    }
}

void CallJava::onCallRenderYUV(int type, int width, int linesize, int height, uint8_t *fy, uint8_t *fu, uint8_t *fv) {
    LOGI("onCallRenderYUV type:%d, width:%d, linesize:%d, height:%d", type, width, linesize, height);
    uint8_t* pFY = fy;
    uint8_t* pFU = fu;
    uint8_t* pFV = fv;
    if (width != linesize) {//对于linesize多余的部分需要进行裁剪
        int ysize = width * height;
        int uvsize = ysize / 4;
        if (!bHasAllocate) {
            pData[0] = static_cast<uint8_t *>(malloc(ysize * sizeof(uint8_t)));
            pData[1] = static_cast<uint8_t *>(malloc(uvsize * sizeof(uint8_t)));
            pData[2] = static_cast<uint8_t *>(malloc(uvsize * sizeof(uint8_t)));
            bHasAllocate = true;
        }

        cutAndCopyYuv(pData, fy, fu, fv, linesize, width, height);

        pFY = pData[0];
        pFU = pData[1];
        pFV = pData[2];
    }

    if (type == MAIN_THREAD) {
        jbyteArray y = jniEnv->NewByteArray(width * height);
        jniEnv->SetByteArrayRegion(y, 0, width * height, reinterpret_cast<const jbyte *>(pFY));

        jbyteArray u = jniEnv->NewByteArray(width * height / 4);
        jniEnv->SetByteArrayRegion(u, 0, width * height / 4, reinterpret_cast<const jbyte *>(pFU));

        jbyteArray v = jniEnv->NewByteArray(width * height / 4);
        jniEnv->SetByteArrayRegion(v, 0, width * height / 4, reinterpret_cast<const jbyte *>(pFV));

        jniEnv->CallVoidMethod(jobj, jmid_renderyuv, width, height, y, u, v);

        jniEnv->DeleteLocalRef(y);
        jniEnv->DeleteLocalRef(u);
        jniEnv->DeleteLocalRef(v);
    } else if (type == CHILD_THREAD) {
        JNIEnv *env;
        if (javaVm->AttachCurrentThread(&env, 0) != JNI_OK) {
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

        env->CallVoidMethod(jobj, jmid_renderyuv, width, height, y, u, v);

        env->DeleteLocalRef(y);
        env->DeleteLocalRef(u);
        env->DeleteLocalRef(v);
        javaVm->DetachCurrentThread();
    }
}

void CallJava::onCallDecodeVPacket(int type, int datasize, uint8_t *packetdata) {
    if (type == MAIN_THREAD) {
        jbyteArray data = jniEnv->NewByteArray(datasize);
        jniEnv->SetByteArrayRegion(data, 0, datasize, reinterpret_cast<const jbyte *>(packetdata));
        jniEnv->CallVoidMethod(jobj, jmid_decodeVPacket, datasize, data);
        jniEnv->DeleteLocalRef(data);
    } else if (type == CHILD_THREAD) {
        JNIEnv *env;
        if (javaVm->AttachCurrentThread(&env, 0) != JNI_OK) {
            if (LOG_DEBUG) {
                LOGE("get child thread jnienv wrong");
                return;
            }
        }

        jbyteArray data = env->NewByteArray(datasize);
        env->SetByteArrayRegion(data, 0, datasize, reinterpret_cast<const jbyte *>(packetdata));
        env->CallVoidMethod(jobj, jmid_decodeVPacket, datasize, data);
        env->DeleteLocalRef(data);
        javaVm->DetachCurrentThread();
    }
}

//裁剪及拷贝yuv数据
void CallJava::cutAndCopyYuv(uint8_t* pData[], uint8_t *srcfy, uint8_t *srcfu, uint8_t *srcfv, int linesize, int width, int height) {
    unsigned char *pDestY = pData[0];
    unsigned char *pDestU = pData[1];
    unsigned char *pDestV = pData[2];
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
