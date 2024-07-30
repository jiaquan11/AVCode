#ifndef CALL_JAVA_H_
#define CALL_JAVA_H_

#include "jni.h"
#include <stdlib.h>
#include <string.h>

#include "log/android_log.h"

class CallJava {
public:
    CallJava(JavaVM *vm, JNIEnv *env, jobject obj);

    ~CallJava();

public:
    void OnCallEglPrepared();

private:
    JavaVM *m_java_vm_ = NULL;
    JNIEnv *m_jni_env_ = NULL;
    jobject m_jobj_ = NULL;
    jmethodID m_jmid_egl_prepared_;
};

#endif //CALL_JAVA_H_