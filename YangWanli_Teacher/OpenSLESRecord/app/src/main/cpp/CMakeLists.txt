cmake_minimum_required(VERSION 3.10.2)

project("openslesrecord")

add_library(native-lib
            SHARED
            native-lib.cpp
            record_double_buffer.cpp
            record_single_buffer.cpp)

find_library(log-lib
             log )

target_link_libraries(native-lib
                      OpenSLES
                      ${log-lib} )