package com.ht.jni;

import android.os.Looper;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.widget.TextView;
import android.widget.Toast;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        int[] i = {11,22,33,44};
        String[] j = {"lei", "hongtai"};
        test(i, j);

        Bean bean = new Bean();
        passBean(bean, "passbean");

//        invokeBean2Method();

        dynamicTest();

        dynamicTest2(100);

        testThread();
    }

    native void test(int[] i, String[] j);

    native void passBean(Bean bean, String str);

    native void invokeBean2Method();

    native void dynamicTest();
    native void dynamicTest2(int i);

    native void testThread();

    public void updateUI() {
        if (Looper.myLooper() == Looper.getMainLooper()){
            Toast.makeText(this,"更新UI",Toast.LENGTH_SHORT).show();
        }else{
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    Toast.makeText(MainActivity.this,"更新UI",Toast.LENGTH_SHORT).show();
                }
            });
        }
    }
}
