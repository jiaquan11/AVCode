package com.jiaquan.myplayer.log;

import android.util.Log;

/**
 * 自定义日志打印类
 */
public class MyLog {
    public static void i(String msg) {
        Log.i("MYPLAYER", msg);
    }

    public static void e(String msg) {
        Log.e("MYPLAYER", msg);
    }
}
