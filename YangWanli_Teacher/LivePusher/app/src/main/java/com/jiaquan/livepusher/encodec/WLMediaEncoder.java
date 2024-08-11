package com.jiaquan.livepusher.encodec;

import android.content.Context;

public class WLMediaEncoder extends WLBaseMediaEncoder {
    private WLEncoderRender mWlEncoderRender_ = null;
    public WLMediaEncoder(Context context) {
        super(context);
        mWlEncoderRender_ = new WLEncoderRender(context);
        setRender(mWlEncoderRender_);
        setRenderMode(WLBaseMediaEncoder.RENDERMODE_WHEN_DIRTY);
    }

    public void setTexture(int textureId, int imageWidth, int imageHeight) {
        if (mWlEncoderRender_ != null) {
            mWlEncoderRender_.setTexture(textureId, imageWidth, imageHeight);
        }
    }
}
