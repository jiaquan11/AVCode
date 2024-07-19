cmake_minimum_required(VERSION 3.10.2)

project("nativeopengldemo")

add_library(native-lib
            SHARED
            native-lib.cpp
            egl/EglHelper.cpp
            egl/EglThread.cpp
            opengl/BaseOpengl.cpp
            opengl/opengl.cpp
            opengl/FilterOne.cpp
            opengl/FilterTwo.cpp
            opengl/FilterYUV.cpp
        )

include_directories(${CMAKE_SOURCE_DIR}/log/)

find_library(log-lib
             log )

target_link_libraries(native-lib
                      android
                      EGL
                      GLESv2
                      ${log-lib} )