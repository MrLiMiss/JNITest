#include <jni.h>
#include <string>

extern "C" JNIEXPORT jstring JNICALL
Java_com_tengfei_jnitest_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++  123123123";
    return env->NewStringUTF(hello.c_str());
}