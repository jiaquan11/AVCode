package com.jiaquan.openglesegl;

import android.os.Bundle;
import android.util.Log;
import android.view.ViewGroup;
import android.widget.LinearLayout;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

public class TextureActivity extends AppCompatActivity {
    private WLGLTextureView wlglTextureView = null;
    private LinearLayout lyContent = null;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_texture);

        wlglTextureView = findViewById(R.id.wlglsurfaceview);
        lyContent = findViewById(R.id.ly_content);

        wlglTextureView.getWlTextureRender().setOnRenderCreateListener(new WLTextureRender.OnRenderCreateListener() {
            @Override
            public void onCreate(final int textid) {//wlglTextureView的渲染器回调一个纹理id过来
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        if (lyContent.getChildCount() > 0) {
                            lyContent.removeAllViews();
                        }

                        //自定义一个线性布局器  多个surface绘制同一个纹理
                        for (int i = 0; i < 3; i++) {
                            WlMutiSurfaceView wlMutiSurfaceView = new WlMutiSurfaceView(TextureActivity.this);//定义三个渲染控件WlMutiSurfaceView
                            wlMutiSurfaceView.setTextureId(textid, i);//设置同一个纹理，即处理同一张图片
                            wlMutiSurfaceView.setSurfaceAndEglContext(null, wlglTextureView.getEglContext());//同时必须在同样的EGL上下文中处理

                            LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
                            lp.width = 200;
                            lp.height = 300;
                            wlMutiSurfaceView.setLayoutParams(lp);
                            lyContent.addView(wlMutiSurfaceView);
                        }
                    }
                });
            }
        });
    }
}
