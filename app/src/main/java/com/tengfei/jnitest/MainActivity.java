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
//        tv.setText(stringFromJNI());
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

    @Override
    public void onClick(View v) {
        switch (v.getId()){
            case R.id.tv_datainit://初始化
                Toast.makeText(MainActivity.this,"dataInit",Toast.LENGTH_SHORT).show();
                break;
            case R.id.tv_datatrack://datatrack
                Toast.makeText(MainActivity.this,"dataTrack",Toast.LENGTH_SHORT).show();
                    break;
            default:
                break;
        }
        //测试埋点相关

    }
}