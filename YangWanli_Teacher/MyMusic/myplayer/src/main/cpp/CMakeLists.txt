cmake_minimum_required(VERSION 3.10.2)

project("myplayer")

include_directories(include)
include_directories(soundtouch/include)
include_directories(soundtouch/SoundTouch)

add_library(native-lib
            SHARED
            native_lib.cpp
            call_java.cpp
            wl_ffmpeg.cpp
            wl_audio.cpp
            wl_packet_queue.cpp
            wl_frame_queue.cpp
            wl_play_status.cpp
            wl_pcm_bean.cpp
            wl_buffer_queue.cpp
            wl_video.cpp
            wl_link_order_queue.cpp
            soundtouch/SoundTouch/AAFilter.cpp
            soundtouch/SoundTouch/FIFOSampleBuffer.cpp
            soundtouch/SoundTouch/FIRFilter.cpp
            soundtouch/SoundTouch/cpu_detect_x86.cpp
            soundtouch/SoundTouch/sse_optimized.cpp
            soundtouch/SoundTouch/RateTransposer.cpp
            soundtouch/SoundTouch/SoundTouch.cpp
            soundtouch/SoundTouch/InterpolateCubic.cpp
            soundtouch/SoundTouch/InterpolateLinear.cpp
            soundtouch/SoundTouch/InterpolateShannon.cpp
            soundtouch/SoundTouch/TDStretch.cpp
            soundtouch/SoundTouch/PeakFinder.cpp)

add_library(avcodec SHARED IMPORTED)
set_target_properties(avcodec
                      PROPERTIES IMPORTED_LOCATION
                      ${CMAKE_SOURCE_DIR}/../nativeLibs/${ANDROID_ABI}/libavcodec.so)

add_library(avdevice SHARED IMPORTED)
set_target_properties(avdevice
                      PROPERTIES IMPORTED_LOCATION
                      ${CMAKE_SOURCE_DIR}/../nativeLibs/${ANDROID_ABI}/libavdevice.so)

add_library(avfilter SHARED IMPORTED)
set_target_properties(avfilter
                      PROPERTIES IMPORTED_LOCATION
                      ${CMAKE_SOURCE_DIR}/../nativeLibs/${ANDROID_ABI}/libavfilter.so)

add_library(avformat SHARED IMPORTED)
set_target_properties(avformat
                      PROPERTIES IMPORTED_LOCATION
                      ${CMAKE_SOURCE_DIR}/../nativeLibs/${ANDROID_ABI}/libavformat.so)

add_library(avutil SHARED IMPORTED)
set_target_properties(avutil
                      PROPERTIES IMPORTED_LOCATION
                      ${CMAKE_SOURCE_DIR}/../nativeLibs/${ANDROID_ABI}/libavutil.so)

add_library(postproc SHARED IMPORTED)
set_target_properties(postproc
                      PROPERTIES IMPORTED_LOCATION
                      ${CMAKE_SOURCE_DIR}/../nativeLibs/${ANDROID_ABI}/libpostproc.so)

add_library(swresample SHARED IMPORTED)
set_target_properties(swresample
                      PROPERTIES IMPORTED_LOCATION
                      ${CMAKE_SOURCE_DIR}/../nativeLibs/${ANDROID_ABI}/libswresample.so)

add_library(swscale SHARED IMPORTED)
set_target_properties(swscale
                      PROPERTIES IMPORTED_LOCATION
                      ${CMAKE_SOURCE_DIR}/../nativeLibs/${ANDROID_ABI}/libswscale.so)

find_library(log-lib
             log)

target_link_libraries(native-lib
                      ${log-lib}
                      avcodec
                      avdevice
                      avfilter
                      avformat
                      avutil
                      postproc
                      swresample
                      swscale
                      OpenSLES
        )