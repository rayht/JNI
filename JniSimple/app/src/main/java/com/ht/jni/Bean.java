package com.ht.jni;

import android.util.Log;

public class Bean {
    private static final String TAG = "Bean-JNI";
    private int i = 100;

    private int getI() {
        return i;
    }

    public void setI(int i) {
        Log.e(TAG,"有人调用了setI");
        this.i = i;
    }


    public static void printInfo(String msg){
        Log.e(TAG,msg);
    }

    public static void printInfo(Bean2 bean){
        Log.e(TAG,"参数是object: "+ bean.i );
    }
}
