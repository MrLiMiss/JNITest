#include <jni.h>
#include <string>
//#include <touch.h>
#include <myTest.h>
#include <touch.h>

// 引入log头文件
#include  <android/log.h>
// log标签
#define  TAG    "这里填写日志的TAG"
// 定义info信息
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,TAG,__VA_ARGS__)
// 定义debug信息
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
// 定义error信息
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,TAG,__VA_ARGS__)

extern "C" JNIEXPORT jstring
Java_com_tengfei_jnitest_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++  123123123";
    return env->NewStringUTF(hello.c_str());
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_tengfei_jnitest_MainActivity_getTime(JNIEnv *env, jobject thiz, jint time) {
    // TODO: implement getTime()
//    int mytime=myGetTime(time);
    int mytime=100;
    return (jint)mytime;
}


extern "C"
JNIEXPORT jint JNICALL
Java_com_tengfei_jnitest_MainActivity_getAge(JNIEnv *env, jobject thiz, jint age) {
    // TODO: implement getAge()
    int myage=getAge(age);
    return myage;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_tengfei_jnitest_HrbbdataUtils_data_1init(JNIEnv *env, jobject thiz, jchar app_url,
                                                  jchar app_key, jchar app_secret, jchar app_enable,
                                                  jchar public_app_name, jchar public_app_version) {
//    return touch_init('http://130.1.10.158:8888/',"","","","",'1.0');
    return 1;
}extern "C"
JNIEXPORT jint JNICALL
Java_com_tengfei_jnitest_MainActivity_dataCreate(JNIEnv *env, jobject thiz) {
    // TODO: implement dataCreate()
    touch_create(3,100,1000,100,50,10);
    return 1;
}extern "C"
JNIEXPORT jint JNICALL
Java_com_tengfei_jnitest_MainActivity_data_1set_1print_1flag(JNIEnv *env, jobject thiz) {
    // TODO: implement data_set_print_flag()
    touch_set_print_flag(1);
    return 1;
}extern "C"
JNIEXPORT jint JNICALL
Java_com_tengfei_jnitest_MainActivity_data_1global_1create(JNIEnv *env, jobject thiz) {
    // TODO: implement data_global_create()
    touch_global_create(3,100,1000,100,50,10);    /*线程池里最小1个线程，最大100个，队列最大值12,每包任务数，失败步长，任务轮询间隔*/
    return 1;
}extern "C"
JNIEXPORT jint JNICALL
Java_com_tengfei_jnitest_MainActivity_data_1global_1init(JNIEnv *env, jobject thiz) {
    // TODO: implement data_global_init()
   return touch_global_init( "http://130.1.10.158:8888/data", "default", "1qweasd!", "true", "app name", "1.0.0");
}extern "C"
JNIEXPORT jint JNICALL
Java_com_tengfei_jnitest_MainActivity_touch_1global_1registerSuperProperties(JNIEnv *env,
                                                                             jobject thiz) {
    // TODO: implement touch_global_registerSuperProperties()

    touch_global_registerSuperProperties( "{\"card_no6\": \"66666666\",\"card_no7\": \"77777777\", \"card_no5\": \"56565656\"}" );
    return 1;
}extern "C"
JNIEXPORT jint JNICALL
Java_com_tengfei_jnitest_MainActivity_data_1track(JNIEnv *env, jobject thiz) {
    // TODO: implement data_track()

    char distinct_id[64+1]={0};
    char properties[128]={0};
    int i=0;

    for(i=1; i<=50; i++) {
        sprintf(distinct_id, "%d", i);
        sprintf(properties, "{\"no\": %d, \"card_no\": \"%06dXXXX\", \"bgint\":1234567890123}", i,i);
        touch_global_track(distinct_id, 1, "AppClick", properties);
        LOGE("AppClick");

    }
    return 1;

}