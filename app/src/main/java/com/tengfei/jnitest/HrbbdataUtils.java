package com.tengfei.jnitest;

/**
 * @ Description :
 * @ Author 李腾飞
 * @ Time 2020/12/28   09:55
 * @ Version :
 */
class HrbbdataUtils {

    static {
        System.loadLibrary("native-lib");
    }

    public native String stringFromJNI();
}
