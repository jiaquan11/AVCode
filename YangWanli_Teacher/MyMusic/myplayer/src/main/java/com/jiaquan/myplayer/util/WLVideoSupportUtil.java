package com.jiaquan.myplayer.util;

import android.media.MediaCodecList;

import java.util.HashMap;
import java.util.Map;

/**
 * 视频解码器支持工具类
 */
public class WLVideoSupportUtil {
    private static Map<String, String> codecMap = new HashMap<>();
    static {//支持H264和H265硬件解码
        codecMap.put("h264", "video/avc");
        codecMap.put("hevc", "video/hevc");
    }

    //返回Android端MediaCodec内部定义的编码器类型
    public static String findVideoCodecName(String ffcodecname){
        if (codecMap.containsKey(ffcodecname)){
            return codecMap.get(ffcodecname);
        }
        return "";
    }

    //判断是否支持硬解指定的解码器格式
    public static boolean isSupportCodec(String ffcodecname){
        boolean supportVideo = false;
        int count = MediaCodecList.getCodecCount();
        for (int i = 0; i < count; i++){
            String[] types = MediaCodecList.getCodecInfoAt(i).getSupportedTypes();//获取MediaCodec支持的所有编解码器类型
            for (int j = 0; j < types.length; j++){
                if (types[j].equals(findVideoCodecName(ffcodecname))){
                    supportVideo = true;
                    break;
                }
            }
            if (supportVideo){
                break;
            }
        }
        return supportVideo;
    }
}
