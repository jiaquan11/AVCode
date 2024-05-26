# AVCode
这里上传记录一些在音视频学习和编写的项目工程的相关工程代码，供今后查阅
---------------------------------------------------------------------------------
杨万里老师课程相关项目
1、JniThread项目
JNI多线程的创建和使用

2、AndroidOpenSLESAudio项目
读取PCM裸数据并调用Android OpenSLES接口进行播放

3、MyMusic项目
FFmpeg打造Android万能音频播放器视频教程，移植FFmpeg到Android平台，利用FFmpeg解码音频数据并用OpenSL ES来播放，打造Android万能播放器。

4、NativeOpenGLDemo项目(Android C++ OpenGL 教程)
掌握Android中OpenGL的原理,学会Android中NDK开发流程,能用OpenGL渲染视频画面,学会OpenGL 中矩阵操作、投影等
用纯C++实现Android的OpenGL渲染功能，包括EGL环境创建、EGL线程创建、OpenGL生命周期管理等，会结合SurfaceView和TextureView实现OpenGL的渲染，
最终我们会实现一个YUV播放器实例，为音视频开发打下坚实的基础。

5、OpenGLDemo项目
使用GLSurfaceView组件进行opengl绘制的demo

6、OpenglESEGL项目
学会自己搭建OpenGL ES的EGL环境和渲染线程控制，各种渲染功能实现（FBO、VBO），多个surface渲染同一个纹理，单个surface渲染多个纹理

7、MyMusic项目(在此项目中增加了视频播放的功能)
FFmpeg+OpenGL ES+OpenSL ES打造Android视频播放器教程，该课程基于C++语言，用FFmpeg、OpenGL ES、OpenSL ES和MediaCodec打造Android视频播放器

8、AudioRecord项目
在Java层调用Android系统接口实现音频录音的测试代码

9、OpenSLESRecord项目
在c++层调用OpenSLES的库接口实现音频录音的测试代码

10、LivePusher项目
摄像头画面方向纠正，摄像头画面编码为MP4视频
摄像头画面和其他音乐合成新的视频，摄像头数据和麦克风音频数据推流到直播服务器实现直播功能
---------------------------------------------------------------------------------
夏曹俊老师课程相关项目
1、testFFmpeg项目
这个是在Android上集成3.4版本ffmpeg的so进行开发测试的Cmake编译的工程项目，调用ffmpeg的基本接口进行打开
读取，软解和硬解操作，使用GLSurface播放组件传入底层进行native windows渲染的流程

2、testOpenSL项目
增加使用opensl播放pcm数据的功能

3、TestOpenGLES项目
增加一个使用OpenGLES读取yuv数据进行渲染的完整功能的测试项目

4、XPlay项目
增加一个完整的在Android端进行视频播放的项目，音频使用opensl进行播放，
视频使用opengles进行渲染，同时使用多种设计模式进行重构