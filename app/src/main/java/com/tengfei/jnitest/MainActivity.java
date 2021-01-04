package com.tengfei.jnitest;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

public class MainActivity extends AppCompatActivity implements View.OnClickListener{

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Example of a call to a native method
        TextView tv = findViewById(R.id.sample_text);
        TextView tv_datainit=findViewById(R.id.tv_datainit);
        TextView tv_dataTrack=findViewById(R.id.tv_datatrack);
        tv.setText(stringFromJNI());
        tv.setText(stringFromJNI()+"   time:"+getTime(100)+"   age:"+getAge(30));
        tv.setOnClickListener(this);
        tv_datainit.setOnClickListener(this);
        tv_dataTrack.setOnClickListener(this);

    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
    public native int getTime(int time);
    public native int getAge(int age);
    public native int data_set_print_flag();//设置是否打印日志
    public native int dataCreate();//touchCreate()

    //touch_global_create(3,100,1000,100,50,10);    /*线程池里最小1个线程，最大100个，队列最大值12,每包任务数，失败步长，任务轮询间隔*/
    public native int data_global_create();

    //touch_global_init( "http://130.1.10.158:8888/data", "default", "1qweasd!", "true", "app name", "1.0.0");
    public native int data_global_init();

    // touch_global_registerSuperProperties( "{\"card_no6\": \"66666666\",\"card_no7\": \"77777777\", \"card_no5\": \"56565656\"}" );
    public native int touch_global_registerSuperProperties();

    public native int data_track();



    @Override
    public void onClick(View v) {
        switch (v.getId()){
            case R.id.tv_datainit://初始化
                Toast.makeText(MainActivity.this,"dataInit",Toast.LENGTH_SHORT).show();
                data_set_print_flag();
                dataCreate();
                data_global_create();
                data_global_init();
                touch_global_registerSuperProperties();
                break;
            case R.id.tv_datatrack://datatrack
                Toast.makeText(MainActivity.this,"dataTrack",Toast.LENGTH_SHORT).show();
                data_track();
                    break;
            default:
                break;
        }
        //测试埋点相关

    }
}