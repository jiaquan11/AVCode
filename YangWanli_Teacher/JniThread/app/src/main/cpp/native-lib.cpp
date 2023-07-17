#include <jni.h>
#include <string>
#include "log/androidLog.h"

#include "pthread.h"

pthread_t thread;

void *normalCallBack(void *data) {
    LOGI("create normal thread from C++");
    pthread_exit(&thread);//退出子线程
}

//创建一般子线程
extern "C" JNIEXPORT void JNICALL
Java_com_jiaquan_jnithread_ThreadDemo_normalThread(JNIEnv *env, jobject thiz) {
    pthread_create(&thread, NULL, normalCallBack, NULL);
}

#include "queue"
#include "unistd.h"
#include "JavaListener.h"

pthread_t product;
pthread_t customer;
pthread_mutex_t mutex;
pthread_cond_t cond;
std::queue<int> queue;
bool isExit = false;

//生产者线程 函数
void *productCallBack(void *data) {
    while (true) {
        pthread_mutex_lock(&mutex);
        queue.push(1);
        pthread_cond_signal(&cond);
        LOGI("生产者生产一个产品，通知消费者消费，产品数量为%d", queue.size());
        pthread_mutex_unlock(&mutex);

        sleep(5);
    }
    pthread_exit(&product);
}

//消费者线程函数
void *customerCallBack(void *data) {
    while (true) {
        pthread_mutex_lock(&mutex);
        if (queue.size() > 0) {
            queue.pop();
            LOGI("消费者消费一个产品，产品数量还剩余%d", queue.size());
        } else {
            LOGI("没有产品可以消费， 等待中....");
            pthread_cond_wait(&cond, &mutex);//等待过程中会自动解锁，然后其它线程可以调度使用
        }
        pthread_mutex_unlock(&mutex);
        usleep(1000 * 500);
    }
    pthread_exit(&customer);
}

//创建生产者和消费者两个子线程
extern "C" JNIEXPORT void JNICALL
Java_com_jiaquan_jnithread_ThreadDemo_mutexThread(JNIEnv *env, jobject thiz) {
    for (int i = 0; i < 10; ++i) {
        queue.push(i);//队列中插入10个数：0-9
    }

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

    pthread_create(&product, NULL, productCallBack, NULL);//创建生产者子线程
    pthread_create(&customer, NULL, customerCallBack, NULL);//创建消费者子线程
}

/*
 * C++调用Java方法
 * */
JavaVM *javaVm;
JavaListener *javaListener;
pthread_t childThread;

//子线程中回调Java方法
void *childCallback(void *data) {
    JavaListener *listener = (JavaListener *) (data);
    listener->onError(0, 101, "C++ call java method from child thread");
    delete listener;
    pthread_exit(&childThread);
}

extern "C" JNIEXPORT void JNICALL
Java_com_jiaquan_jnithread_ThreadDemo_callBackFromC(JNIEnv *env, jobject thiz) {
    javaListener = new JavaListener(javaVm, env, env->NewGlobalRef(thiz));//NewGlobalRef 全局引用
//    javaListener->onError(1, 100, "C++ call java method from main thread"); //主线程中回调Java方法

    pthread_create(&childThread, NULL, childCallback, javaListener);//子线程中回调Java方法
}

//JVM自动调用加载函数，在Java层调用System.loadLibrary()函数会自动调用JNI_OnLoad方法
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    javaVm = vm;
    if (javaVm->GetEnv((void **) (&env), JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }
    return JNI_VERSION_1_6;
}