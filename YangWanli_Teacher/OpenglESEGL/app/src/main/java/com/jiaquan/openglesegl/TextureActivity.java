package com.jiaquan.openglesegl;

import android.os.Bundle;
import android.view.ViewGroup;
import android.widget.LinearLayout;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

public class TextureActivity extends AppCompatActivity {
    private WLGLTextureView mWlglTextureView_ = null;
    private LinearLayout mLyContent_ = null;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_texture);

        mWlglTextureView_ = findViewById(R.id.wlglsurfaceview);
        mLyContent_ = findViewById(R.id.ly_content);//一个线性布局器
        mWlglTextureView_.getWlTextureRender().setOnRenderCreateListener(new WLTextureRender.OnRenderCreateListener() {
            @Override
            public void onCreate(final int textureId) {
                /**
                 * runOnUiThread是一个常用的方法，它允许从任何线程将代码块调度到主线程（UI线程）中运行。
                 * 用于确保在非UI线程（如后台线程）中运行的代码能够安全地更新UI.
                 */
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        if (mLyContent_.getChildCount() > 0) {
                            mLyContent_.removeAllViews();
                        }
                        /**
                         * 自定义一个线性布局器,添加多个surfaceview控件，用来绘制同一个纹理
                         * 显示不同的纹理效果，即显示同一张图片的不同效果
                         */
                        for (int i = 0; i < 3; i++) {
                            WlMutiSurfaceView wlMutiSurfaceView = new WlMutiSurfaceView(TextureActivity.this);
                            wlMutiSurfaceView.setTextureId(textureId, i);//设置同一个纹理，即处理同一张图片(多个Surface绘制同一个纹理)
                            /**
                             * 如果多个SurfaceView组件共享相同的EGLContext，它们可以共享OpenGL对象（如纹理、帧缓冲区等）。
                             * 可以避免在每个SurfaceView中为同一个资源重复创建多个副本，从而节约内存和资源加载时间
                             * 共享的EGLContext包括哪些内容:
                             * 纹理（Textures）：
                             * 共享的EGLContext允许不同的绘图环境使用相同的纹理对象。这样可以避免重复加载相同的纹理图像，提高内存使用效率和加载速度。
                             * 缓冲区对象（Buffers）：
                             * 包括顶点缓冲区对象（VBO）、元素缓冲区对象（EBO）等，这些缓冲区可以存储顶点数据、索引数据，并在多个绘图环境中共享，减少资源冗余。
                             * 着色器程序（Shader Programs）：
                             * 共享 EGLContext 可以使多个绘图环境使用相同的着色器程序，避免重新编译和链接着色器，提高了效率。
                             * 帧缓冲区对象（Frame Buffer Objects，FBOs）：
                             * 通过共享 EGLContext 可以让不同绘图目标重用于同一个帧缓冲区对象，使得渲染过程更灵活。
                             * 渲染缓冲区对象（Render Buffer Objects，RBOs）：
                             * 渲染缓冲区对象用于离屏渲染，同样可以在共享EGLContext下被多目标复用。
                             * 其他OpenGLES对象:
                             * 共享的EGLContext还包括一些其他的OpenGLES资源，如查询对象、同步对象等。
                             */
                            wlMutiSurfaceView.setSurfaceAndEglContext(null, mWlglTextureView_.getEglContext());//同时必须在同样的EGL上下文中处理

                            LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
                            lp.width = 200;
                            lp.height = 300;
                            wlMutiSurfaceView.setLayoutParams(lp);//设置自定义SurfaceView组件的宽高
                            mLyContent_.addView(wlMutiSurfaceView);//将自定义SurfaceView组件添加到线性布局器中
                        }
                    }
                });
            }
        });
    }
}
