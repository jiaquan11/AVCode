#ifndef _JAVALISTENER_H_
#define _JAVALISTENER_H_

#include "jni.h"

class JavaListener {
public:
    JavaListener(JavaVM *vm, JNIEnv *env, jobject obj);

    ~JavaListener();

public:
    /**
     * 1:主线程
     * 0：子线程
     * @param threadType
     * @param code
     * @param msg
     */
    void onError(int threadType, int code, const char *msg);

public:
    JavaVM *jvm;
    jobject jobj;
    JNIEnv *jniEnv;

    jmethodID jmethodId;
};

#endif
