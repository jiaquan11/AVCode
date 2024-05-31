package com.jiaquan.myplayer.util;

import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.os.Build;

import androidx.annotation.RequiresApi;

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

    public static String findVideoCodecType(String codecTag) {
        if (codecMap.containsKey(codecTag)) {
            String codecType = codecMap.get(codecTag);
            return codecType;
        }
        return "";
    }

    /**
     * 判断是否支持指定的编解码器
     * 说明:
     * codecName:编解码器详细名称
     * 例如：c2.qti.avc.decoder  OMX.qcom.video.decoder.avc   c2.qti.avc.decoder.low_latency  OMX.qcom.video.decoder.avc.low_latency
     * codecType:编解码器类型
     * 例如：video/avc
     * 上述每个编解码器的名称都对应一个编码器类型，例如c2.qti.avc.decoder对应video/avc，所以上述的编解码器名称对应的编码器类型都是video/avc
     */
    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
    public static String getHardwareDecoderName(String codecTag) {
        String mimeType = findVideoCodecType(codecTag);
        if (mimeType.equals("")) {
            return "";
        }

        MediaCodecList list = new MediaCodecList(MediaCodecList.REGULAR_CODECS);
        MediaCodecInfo[] codecs = list.getCodecInfos();
        for (MediaCodecInfo codec : codecs) {
            if (codec.isEncoder()) {//过滤掉编码器
                continue;
            }
            for (String type : codec.getSupportedTypes()) {
                if (!type.equalsIgnoreCase(mimeType)) {//匹配到解码器类型
                    continue;
                }

                // 匹配到指定的mimeType:video/avc或者video/hevc
                // Android高版本系统直接调用isHardwareAccelerated()判断
                if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.Q) {//Android10=SDK29
                    if (!codec.isHardwareAccelerated()) {
                        continue;
                    }

                    boolean isSupport = _detectionCreateHardDecoder(mimeType);//创建指定解码器检测
                    if (!isSupport) {
                        MyLog.e("create higher hard decoder failed");
                        return "";
                    }
                    MyLog.i("Found av_player(Android High Version) Hardware Decoder: " + codec.getName());
                    return codec.getName();//返回硬解码器名称，例如OMX.qcom.video.decoder.avc，取支持的第一个
                }

                //Android低版本系统解码器判断逻辑
                String name = codec.getName();
                if (name.startsWith("OMX.google.") || name.startsWith("c2.android.") ||
                        ((!name.startsWith("OMX.")) && (!name.startsWith("c2.")))) {//软解
                    MyLog.i("Found av_player(Android) software decoder: " + name);
                    return "";
                } else {//硬解
                    boolean isSupport = _detectionCreateHardDecoder(mimeType);
                    if (!isSupport) {
                        MyLog.e("create lower hard decoder failed");
                        return "";
                    }
                    MyLog.i("Found av_player(Android lower Version) Hardware Decoder: " + name);
                    return name;
                }
            }
        }
        MyLog.e("the device found no decoder of " + mimeType);
        return "";
    }

    private static boolean _detectionCreateHardDecoder(String mimeType) {
        boolean isSupport = false;
        MediaCodec decoder = null;
        try {
            decoder = MediaCodec.createDecoderByType(mimeType);
            isSupport = true;
        } catch (Throwable e) {
            e.printStackTrace();
            isSupport = false;
        } finally {
            if (decoder != null) {
                decoder.release();
                decoder = null;
            }
        }
        return isSupport;
    }
}
