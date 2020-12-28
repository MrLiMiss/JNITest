#include <jni.h>
#include <string>

extern "C" JNIEXPORT jstring JNICALL
Java_com_tengfei_jnitest_MainActivity_stringFromJNITest(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++  Test happy";
    return env->NewStringUTF(hello.c_str());
}