#include "java_listener.h"

java_listener::java_listener(JavaVM *vm, JNIEnv *env, jobject obj) {
    m_jvm_ = vm;
    m_jobj_ = obj;
    m_jniEnv_ = env;

    jclass clz = env->GetObjectClass(m_jobj_);
    if (!clz) {
        return;
    }
    m_jmethod_id_ = env->GetMethodID(clz, "onError", "(ILjava/lang/String;)V");
}

java_listener::~java_listener() {
    if (m_jobj_ != nullptr) {
        m_jniEnv_->DeleteGlobalRef(m_jobj_);
        m_jobj_ = nullptr;
    }
}

void java_listener::OnError(int threadType, int code, const char *msg) {
    if (threadType == 0) {//子线程
        JNIEnv *env;
        m_jvm_->AttachCurrentThread(&env, 0);
        jstring jmsg_str = env->NewStringUTF(msg);
        env->CallVoidMethod(m_jobj_, m_jmethod_id_, code, jmsg_str);
        env->DeleteLocalRef(jmsg_str);
        m_jvm_->DetachCurrentThread();
    } else if (threadType == 1) {//主线程
        jstring jmsg_str = m_jniEnv_->NewStringUTF(msg);
        m_jniEnv_->CallVoidMethod(m_jobj_, m_jmethod_id_, code, jmsg_str);
        m_jniEnv_->DeleteLocalRef(jmsg_str);
    }
}
