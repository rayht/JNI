#include <jni.h>
#include <string>
#include <android/log.h>
#include <pthread.h>

#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,"JNI",__VA_ARGS__);

/**
 * 动态注册方法
 * @param env
 * @param instance
 */
void dynamicTest(JNIEnv *env, jobject instance) {
    LOGE("dynamicTest");
}

extern "C"
/**
 * @param env
 *  C++ : _JNIEnv结构体， env一级指针，调用的时候只需要env->XXX即可
 *  C   ：JNINativeInterface* 变量，即JNINativeInterface的指针，*env为二级指针，所以调用时需要(*env)->XXX
 * @param instance
 *  调用native方法的实例，如在MainActivity中调用native方法，instance=MainActivity.this
 * @param i_
 *  传入参数，int[]
 * @param j
 *  传入参数, String[]
 */
JNIEXPORT void JNICALL
Java_com_ht_jni_MainActivity_test(JNIEnv *env, jobject instance, jintArray i_,
                                         jobjectArray j) {

    /**
     * JNI int 数组操作
     */
    jint *i = env->GetIntArrayElements(i_, NULL);
    int32_t length = env->GetArrayLength(i_);
    for(int32_t k = 0; k < length; k++) {
        __android_log_print(ANDROID_LOG_ERROR, "JNI", "数组值=%d", *(i+k));
        //修改值
        *(i + k) = 100;
    }

    /**
     * 参数3， model
     * 0:  刷新java数组 并 释放c/c++数组
     * 1 = JNI_COMMIT: 只刷新java数组
     * 2 = JNI_ABORT:只释放
     */
    env->ReleaseIntArrayElements(i_, i, 0);


    /**
     * JNI操作String数组
     */
    int32_t str_length = env->GetArrayLength(j);
    for(int i = 0; i < str_length; i++) {
        jstring j_str = static_cast<jstring>(env->GetObjectArrayElement(j, i));
        const char *c_str = env->GetStringUTFChars(j_str, 0);
        __android_log_print(ANDROID_LOG_ERROR, "JNI", "str=%s", c_str);

        env->ReleaseStringUTFChars(j_str, c_str);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ht_jni_MainActivity_passBean(JNIEnv *env, jobject instance, jobject bean,
                                             jstring str_) {
    const char *str = env->GetStringUTFChars(str_, 0);

    /**
     * Native调用java的类及方法
     */
    //1. 获得Java的类对象
    jclass jcls_bean = env->GetObjectClass(bean);

    /**
     * 2. 查找要调用的方法
     * 参数：
     *  jcls_bean : 对应的java class对象
     *  "getI" ：方法名
     *  "()I")　：方法签名
     *  详情如下：
     *  Java类型	    签名
     *   boolean	     Z
     *   short	     S
     *   float	     F
     *   byte	     B
     *   int	         I
     *   double	     D
     *   char	     C
     *   long	     J
     *   void	     V
     *   引用类型    L + 全限定名 + ; 注意：分号必须加到最后面，全限定名用/分隔而不是.
     *   数组	    [+类型签名
     *
     *   或者使用javap命令生成，然后直接copy避免出错
     *   app\build\intermediates\classes\debug>javap -s com.ht.jni.Bean
     *
     */
    jmethodID jmethod_getI = env->GetMethodID(jcls_bean, "getI", "()I");

    /**
     * 3. 调用Java方法
     * 参数：
     * bean: 在哪个对象上调用方法，这里即bean
     * jmethod_getI : jmethodID,第2步中获取的方法Id
     */
    jint ret_getI = env->CallIntMethod(bean, jmethod_getI);
    __android_log_print(ANDROID_LOG_ERROR, "JNI", "C++调用Java : ret_getI=%d", ret_getI);

    jmethodID jmethod_setI = env->GetMethodID(jcls_bean, "setI", "(I)V");
    env->CallVoidMethod(bean, jmethod_setI, 200);

    jmethodID jmethod_static_printfInfo = env->GetStaticMethodID(jcls_bean, "printInfo", "(Ljava/lang/String;)V");
    //创建Java String
    jstring jstring_msg = env->NewStringUTF("C++调用Java static方法");
    env->CallStaticVoidMethod(jcls_bean, jmethod_static_printfInfo, jstring_msg);
    //释放局部变量
    env->DeleteLocalRef(jstring_msg);

    /**
     * 参数为Object对象的方法调用
     */
    //1. 获得Java的类对象,如上已经获取 jcls_bean

    //2. 获取方法Id
    jmethodID jmethod_printInfo2 = env->GetStaticMethodID(jcls_bean, "printInfo", "(Lcom/ht/jni/Bean2;)V");
    /**
     * 3. 反射创建对象
     */
    //3.1. 获取Java的Class对象
    jclass jclass_bean2 = env->FindClass("com/ht/jni/Bean2");
    //3.2 获取构造方法
    jmethodID jmethod_constractor = env->GetMethodID(jclass_bean2, "<init>", "(I)V");
    //3.2. 调用构造方法创建对象实例
    jobject bean2 = env->NewObject(jclass_bean2, jmethod_constractor, 88);

    //4. 调用方法
    env->CallStaticVoidMethod(jcls_bean, jmethod_printInfo2, bean2);

    //5. 释放反射创建的对象及jclass_bean2
    env->DeleteLocalRef(bean2);
    env->DeleteLocalRef(jclass_bean2);


    /**
     * 访问及修改对象的属性
     */
    jfieldID jfield_i = env->GetFieldID(jcls_bean, "i", "I");
    env->SetIntField(bean, jfield_i, 500);


    env->ReleaseStringUTFChars(str_, str);
}

jobject jbean2;
jclass jclass_bean2;
jclass jclass_bean2_weak;
extern "C"
JNIEXPORT void JNICALL
Java_com_ht_jni_MainActivity_invokeBean2Method(JNIEnv *env, jobject instance) {

    if(jclass_bean2 == NULL) {
        //这里如果重复调用方法会出现问题
        //jclass_bean2为局部引用，方法执行完后会自动释放。
        //导致jclass_bean2指针还存在，但是指针指向的地址的值 已经不存了，导致悬空指针的产生
        jclass jclass_bean2_local = env->FindClass("com/ht/jni/Bean2");

        //把它变成全局引用就会避免悬空指针的产生
        jclass_bean2 = static_cast<jclass>(env->NewGlobalRef(jclass_bean2_local));

        //释放局部引用
        env->DeleteLocalRef(jclass_bean2_local);
        //必要的时候释放全局引用，一般在release/stop/reset等方法中释放全局引用
        env->DeleteGlobalRef(jclass_bean2);

        /**
         * 弱全局引用
         *  与Java类似，弱全局引用不会阻止内存的回收
         */
        jclass_bean2_weak = static_cast<jclass>(env->NewWeakGlobalRef(jclass_bean2));

        //所以我们在使用弱全局引用之前需要判断弱全局引用是否还可用
        //true:已经释放
        //false:还没有释放，可以使用
        jboolean is_ = env->IsSameObject(jclass_bean2_weak, NULL);
    }
}


/**
 * 动态注册方法
 * @param env
 * @param instance
 * @param i
 */
void dynamicTest2(JNIEnv *env, jobject instance, jint i) {
    LOGE("dynamicTest, i = %d", i);
}

/**
 * 动态注册方法映射表[数组]
 *
 * {"Java类中的方法名"， "方法签名", "C/C++对应方法名，必须转成void*"}
 */
static const JNINativeMethod methods[] = {
        {"dynamicTest", "()V", (void*)dynamicTest},
        {"dynamicTest2", "(I)V", (void*)dynamicTest2}
};

/**
 * 动态注册对应的Java类全限定名
 */
static const char* cls_name = "com/ht/jni/MainActivity";

JavaVM *_vm;

/**
 * 执行System.loadLibrary第一个自动调用的方法,可在这个方法里面动态注册native方法
 * @param vm
 * @param re
 * @return
 */
int JNI_OnLoad(JavaVM *vm, void *re) {
    LOGE("JNI_OnLoad...");

    _vm = vm;

    //获取env
    JNIEnv *env = nullptr;
    jint r = _vm->GetEnv((void **)&env, JNI_VERSION_1_6);
    if(r != JNI_OK) {
        LOGE("dynamic register error, obtain JNIEnv error!");
        return -1;
    }

    //获取Class对象
    jclass jcls_dynamic = env->FindClass(cls_name);

    //动态注册native方法
    env->RegisterNatives(jcls_dynamic, methods, sizeof(methods) / sizeof(JNINativeMethod));
    return JNI_VERSION_1_6;
}

jobject reference_this;

void* threadTask(void *args) {
    //在线程里面获取env
    JNIEnv *env;
    //将Native线程附加到Java虚拟机，以获取JNIEnv
    _vm->AttachCurrentThread(&env, 0);

    //执行耗时操作

    //反射调用Java的方法
    jclass jcls_this = env->GetObjectClass(reference_this);
    jmethodID  jmethod_updateUI = env->GetMethodID(jcls_this, "updateUI", "()V");
    env->CallVoidMethod(reference_this, jmethod_updateUI);

    //释放全局变量
    env->DeleteGlobalRef(reference_this);
    reference_this = 0;

    //分离线程与虚拟机
    _vm->DetachCurrentThread();

    return 0;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ht_jni_MainActivity_testThread(JNIEnv *env, jobject instance) {

    pthread_t pid;
    reference_this = env->NewGlobalRef(instance);
    pthread_create(&pid, 0, threadTask, reference_this);
}