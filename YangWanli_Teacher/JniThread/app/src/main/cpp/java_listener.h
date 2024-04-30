#ifndef JNITHREAD_JAVALISTENER_H_
#define JNITHREAD_JAVALISTENER_H_

#include "jni.h"

class java_listener {
public:
    java_listener(JavaVM *vm, JNIEnv *env, jobject obj);
    ~java_listener();

public:
    //0: 子线程 1: 主线程
    void OnError(int threadType, int code, const char *msg);

private:
    JavaVM *m_jvm_ = nullptr;
    jobject m_jobj_ = nullptr;
    JNIEnv *m_jniEnv_ = nullptr;
    jmethodID m_jmethod_id_ = nullptr;
};

#endif //JNITHREAD_JAVALISTENER_H_
