package com.jiaquan.myplayer.util;

import android.media.MediaCodecInfo;
import android.media.MediaCodecList;

import com.jiaquan.myplayer.log.MyLog;

import java.util.HashMap;
import java.util.Map;

/**
 * 视频解码器支持工具类
 */
public class WLVideoSupportUtil {
    private static Map<String, String> codecMap = new HashMap<>();

    static {
        codecMap.put("h264", "video/avc");
        codecMap.put("hevc", "video/hevc");
    }

    public static String findVideoCodecName(String codecTag) {
        if (codecMap.containsKey(codecTag)) {
            String codecType = codecMap.get(codecTag);
            return codecType;
        }
        return "";
    }

    /**
     * 判断是否支持指定的编解码器
     说明:
     codecName:编解码器详细名称
     例如：c2.qti.avc.decoder  OMX.qcom.video.decoder.avc   c2.qti.avc.decoder.low_latency  OMX.qcom.video.decoder.avc.low_latency
     codecType:编解码器类型
     例如：video/avc
     上述每个编解码器的名称都对应一个编码器类型，例如c2.qti.avc.decoder对应video/avc，所以上述的编解码器名称对应的编码器类型都是video/avc
     */
    public static boolean isSupportCodec(String codecTag) {
        boolean supportVideo = false;
        int count = MediaCodecList.getCodecCount();//获取MediaCodec支持的所有编解码器数量
        for (int i = 0; i < count; i++) {
            MediaCodecInfo codecInfo = MediaCodecList.getCodecInfoAt(i);
            String codecName = codecInfo.getName();
            MyLog.i("isSupportCodec name:" + codecName);
            String[] types = MediaCodecList.getCodecInfoAt(i).getSupportedTypes();//获取指定索引的编解码器支持的所有类型
            for (int j = 0; j < types.length; j++) {
                MyLog.i("isSupportCodec types:" + types[j]);
            }
            for (int j = 0; j < types.length; j++) {
                if (types[j].equals(findVideoCodecName(codecTag))) {
                    supportVideo = true;
                    break;
                }
            }
            if (supportVideo) {
                break;
            }
        }
        return supportVideo;
    }
}
