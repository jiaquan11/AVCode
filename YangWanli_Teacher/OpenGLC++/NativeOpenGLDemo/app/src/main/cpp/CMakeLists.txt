cmake_minimum_required(VERSION 3.10.2)

project("nativeopengldemo")

add_library(native-lib
            SHARED
            native-lib.cpp
            egl/egl_helper.cpp
            egl/egl_thread.cpp
            opengl/base_opengl.cpp
            opengl/opengl.cpp
            opengl/filter_one.cpp
            opengl/filter_two.cpp
            opengl/filter_yuv.cpp
        )

include_directories(${CMAKE_SOURCE_DIR}/log/)

find_library(log-lib
             log )

target_link_libraries(native-lib
                      android
                      EGL
                      GLESv2
                      ${log-lib} )