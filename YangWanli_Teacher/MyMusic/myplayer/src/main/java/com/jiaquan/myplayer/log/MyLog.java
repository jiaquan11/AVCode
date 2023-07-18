package com.jiaquan.myplayer.log;

import android.util.Log;

/*
* 日志类封装
* 统一了指定的TAG
* */
public class MyLog {
    public static void i(String msg) {
        Log.i("MYPLAYER", msg);
    }

    public static void e(String msg) {
        Log.e("MYPLAYER", msg);
    }
}
