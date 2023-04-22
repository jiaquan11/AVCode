cmake_minimum_required(VERSION 3.4.1)

#添加头文件路径(相对于本文件路径)
include_directories(include)

#设置ffmpeg库所在路径的变量
set(FF ${CMAKE_CURRENT_SOURCE_DIR}/libs/${ANDROID_ABI})
add_library(avcodec SHARED IMPORTED)
set_target_properties(avcodec PROPERTIES IMPORTED_LOCATION ${FF}/libavcodec.so)

add_library(avformat SHARED IMPORTED)
set_target_properties(avformat PROPERTIES IMPORTED_LOCATION ${FF}/libavformat.so)

add_library(avutil SHARED IMPORTED)
set_target_properties(avutil PROPERTIES IMPORTED_LOCATION ${FF}/libavutil.so)

add_library(swscale SHARED IMPORTED)
set_target_properties(swscale PROPERTIES IMPORTED_LOCATION ${FF}/libswscale.so)

add_library(swresample SHARED IMPORTED)
set_target_properties(swresample PROPERTIES IMPORTED_LOCATION ${FF}/libswresample.so)

add_library(native-lib
             SHARED
             native-lib.cpp
             IDemux.cpp
             FFDemux.cpp
             XData.cpp
             XThread.cpp
             IObserver.cpp
             IDecode.cpp
             FFDecode.cpp
             IVideoView.cpp
             GLVideoView.cpp
             XTexture.cpp
             XEGL.cpp
             XShader.cpp
             IResample.cpp
             FFResample.cpp
             IAudioPlay.cpp
             SLAudioPlay.cpp
             IPlayer.cpp
             IPlayerBuilder.cpp
             FFPlayerBuilder.cpp
             IPlayerProxy.cpp
        )

find_library(log-lib
              log )

target_link_libraries(native-lib
                        GLESv2
                        EGL
                        android
                        OpenSLES
                        avcodec
                        avformat
                        avutil
                        swscale
                        swresample
                       ${log-lib} )