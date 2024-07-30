#include "call_java.h"

CallJava::CallJava(JavaVM *vm, JNIEnv *env, jobject obj) {
    m_java_vm_ = vm;
    m_jni_env_ = env;
    m_jobj_ = env->NewGlobalRef(obj);//创建全局引用,防止被回收,可以跨线程使用
    jclass clz = m_jni_env_->GetObjectClass(m_jobj_);
    if (clz == NULL) {
        LOGE("CallJava get jclass wrong");
        return;
    }
    m_jmid_egl_prepared_ = env->GetMethodID(clz, "onCallEglPrepared", "()V");
}

CallJava::~CallJava() {
    LOGI("CallJava::~CallJava in");
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

void CallJava::OnCallEglPrepared() {
    JNIEnv *env;
    if (m_java_vm_->AttachCurrentThread(&env, 0) != JNI_OK) {
        LOGE("get child thread jnienv wrong");
        return;
    }
    env->CallVoidMethod(m_jobj_, m_jmid_egl_prepared_);
    m_java_vm_->DetachCurrentThread();
}
