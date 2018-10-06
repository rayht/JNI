/**
 * @Author:Leiht
 * @Date:2018/10/1
 */

#include <jni.h>
#include "JavaCallHelper.h"
#include "macro.h"

JavaCallHelper::JavaCallHelper(JavaVM *vm, JNIEnv *env, jobject instance) {
    this->javaVM = vm;
    this->env = env;
    // 涉及到jobject 跨方法 跨线程 就需要创建全局引用
    this->instance = env->NewGlobalRef(instance);

    jclass clazz = env->GetObjectClass(this->instance);
    onErrorId = env->GetMethodID(clazz, "onError", "(I)V");
    onPrepareId = env->GetMethodID(clazz, "onPrepare", "()V");
    onProgressId = env->GetMethodID(clazz, "onProgress", "(I)V");
}

JavaCallHelper::~JavaCallHelper() {
    //释放全局变量
    env->DeleteGlobalRef(this->instance);
}

void JavaCallHelper::onError(int thread, int errorCode) {
    if(thread == THREAD_MAIN) {
        env->CallVoidMethod(this->instance, onErrorId, errorCode);
    }else {
        JNIEnv *p_env;
        javaVM->AttachCurrentThread(&p_env, 0);
        p_env->CallVoidMethod(this->instance, onErrorId, errorCode);
        javaVM->DetachCurrentThread();
        DELETE(p_env);
    }
}

void JavaCallHelper::onParpare(int thread) {
    if(thread == THREAD_MAIN) {
        env->CallVoidMethod(this->instance, onPrepareId);
    }else {
        JNIEnv *p_env;
        javaVM->AttachCurrentThread(&p_env, 0);
        p_env->CallVoidMethod(this->instance, onPrepareId);
        javaVM->DetachCurrentThread();
        DELETE(p_env);
    }
}

void JavaCallHelper::onProgress(int thread, int progress) {
    if (thread == THREAD_CHILD) {
        JNIEnv *jniEnv;
        if (javaVM->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
            return;
        }
        jniEnv->CallVoidMethod(instance, onProgressId, progress);
        javaVM->DetachCurrentThread();
    } else {
        env->CallVoidMethod(instance, onProgressId, progress);
    }
}

