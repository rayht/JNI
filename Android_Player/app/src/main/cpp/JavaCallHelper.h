/**
 * @Author:Leiht
 * @Date:2018/10/1
 */

#ifndef ANDROID_PLAYER_JAVACALLHELPER_H
#define ANDROID_PLAYER_JAVACALLHELPER_H

#include <jni.h>

class JavaCallHelper {
private:
    JavaVM *javaVM = 0;

    JNIEnv *env = 0;

    jobject instance;

    jmethodID onErrorId;

    jmethodID onPrepareId;

    jmethodID onProgressId;
public:
    JavaCallHelper(JavaVM *vm, JNIEnv *env, jobject instance);

    ~JavaCallHelper();

    void onError(int thread, int errorCode);

    void onParpare(int thread);

    void onProgress(int thread, int progress);
};

#endif //ANDROID_PLAYER_JAVACALLHELPER_H
