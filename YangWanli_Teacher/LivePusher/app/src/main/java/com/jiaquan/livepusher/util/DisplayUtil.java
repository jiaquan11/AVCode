package com.jiaquan.livepusher.util;

import android.content.Context;
import android.util.DisplayMetrics;

/*
* 手机屏幕操作工具类
* */
public class DisplayUtil {
    //获取手机屏幕显示宽高
    public static int getScreenWidth(Context context) {
        DisplayMetrics metrics = context.getResources().getDisplayMetrics();
        return metrics.widthPixels;
    }

    public static int getScreenHeight(Context context) {
        DisplayMetrics metrics = context.getResources().getDisplayMetrics();
        return metrics.heightPixels;
    }
}
