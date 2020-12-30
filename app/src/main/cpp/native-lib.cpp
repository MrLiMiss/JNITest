#include <jni.h>
#include <string>
//#include <touch.h>
#include <myTest.h>


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





//extern "C"
//JNIEXPORT jint JNICALL
//Java_com_tengfei_jnitest_HrbbdataUtils_data_1registerSuperProperties__(JNIEnv *env, jobject thiz) {
//    // TODO: implement data_registerSuperProperties()
//}extern "C"
//JNIEXPORT jint JNICALL
//Java_com_tengfei_jnitest_HrbbdataUtils_data_1track(JNIEnv *env, jobject thiz) {
//    // TODO: implement data_track()
//}extern "C"
//JNIEXPORT jint JNICALL
//Java_com_tengfei_jnitest_HrbbdataUtils_data_1trackSignUp(JNIEnv *env, jobject thiz) {
//    // TODO: implement data_trackSignUp()
//}
extern "C"
JNIEXPORT jint JNICALL
Java_com_tengfei_jnitest_HrbbdataUtils_data_1init(JNIEnv *env, jobject thiz, jchar app_url,
                                                  jchar app_key, jchar app_secret, jchar app_enable,
                                                  jchar public_app_name, jchar public_app_version) {
//    return touch_init('http://130.1.10.158:8888/',"","","","",'1.0');
    return 1;
}