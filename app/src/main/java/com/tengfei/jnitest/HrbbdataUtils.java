package com.tengfei.jnitest;

/**
 * @ Description :
 * @ Author 李腾飞
 * @ Time 2020/12/28   09:55
 * @ Version :
 */
class HrbbdataUtils {

    static {
        System.loadLibrary("data-lib");
    }

    //初始化
    public native int data_init(char app_url, char app_key, char app_secret, char app_enable, char public_app_name, char public_app_version);

//    //设置所有埋点数据中的公共属性
//    public native int data_registerSuperProperties();
//    //事件埋点信息添加到内存队列中
//    public native int data_track();
//    //登录id和匿名id绑定(记录用户注册事件)
//    public native int data_trackSignUp();


}
