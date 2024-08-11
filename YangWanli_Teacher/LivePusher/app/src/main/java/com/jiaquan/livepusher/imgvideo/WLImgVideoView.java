package com.jiaquan.livepusher.imgvideo;

import android.content.Context;
import android.util.AttributeSet;

import com.jiaquan.livepusher.egl.WLEGLSurfaceView;

public class WLImgVideoView extends WLEGLSurfaceView {
    private WLImgVideoRender mWlImgVideoRender_ = null;
    private int mFboTextureId_ = 0;
    private int mSurfaceWidth_ = 0;
    private int mSurfaceHeight_ = 0;

    private OnEncodeListener mOnEncodeListener_;
    public void setOnEncodeListener(OnEncodeListener onEncodeListener) {
        mOnEncodeListener_ = onEncodeListener;
    }
    public interface OnEncodeListener {
        void onEncode();
    }

    public WLImgVideoView(Context context) {
        this(context, null);
    }

    public WLImgVideoView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public WLImgVideoView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        mWlImgVideoRender_ = new WLImgVideoRender(context);
        setRender(mWlImgVideoRender_);
        setRenderMode(WLEGLSurfaceView.RENDERMODE_WHEN_DIRTY);
        mWlImgVideoRender_.setOnRenderCreateListener(new WLImgVideoRender.OnRenderCreateListener() {
            @Override
            public void onCreate(int textureId, int surfaceWidth, int surfaceHeight) {
                mFboTextureId_ = textureId;
                mSurfaceWidth_ = surfaceWidth;
                mSurfaceHeight_ = surfaceHeight;
            }
        });
        mWlImgVideoRender_.setOnEncodeListener(new WLImgVideoRender.OnEncodeListener() {
            @Override
            public void onEncode() {
                if (mOnEncodeListener_ != null) {
                    mOnEncodeListener_.onEncode();
                }
            }
        });
    }

    public void setCurrentImg(int imageId) {
        if (mWlImgVideoRender_ != null) {
            mWlImgVideoRender_.setCurrentImgageId(imageId);
            requestRender();
        }
    }

    public int getFboTextureId() {
        return mFboTextureId_;
    }

    public int getFboWidth() {
        return mSurfaceWidth_;
    }

    public int getFboHeight() {
        return mSurfaceHeight_;
    }
}
