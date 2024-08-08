package com.jiaquan.livepusher.util;

import android.content.Context;
import android.util.DisplayMetrics;

/**
 * 屏幕工具类
 */
public class DisplayUtil {
    public static int getScreenWidth(Context context) {
        DisplayMetrics metrics = context.getResources().getDisplayMetrics();
        return metrics.widthPixels;
    }

    public static int getScreenHeight(Context context) {
        DisplayMetrics metrics = context.getResources().getDisplayMetrics();
        return metrics.heightPixels;
    }
}
