cmake_minimum_required(VERSION 3.4.1)

file(GLOB SOURCE src/main/cpp/*.cpp )
add_library(
             native-lib
             SHARED
             ${SOURCE})

include_directories(src/main/cpp/include)

#设置CMAKE_CXX_FLAGS，传递给C++编译器，-L：lib查找库
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__STDC_CONSTANT_MACROS -L${CMAKE_SOURCE_DIR}/src/main/cpp/libs/${ANDROID_ABI}")

#find_library(log-lib log z android OpenSLES)

target_link_libraries(
                       native-lib
                       avformat avcodec avfilter    avutil swresample swscale
                       log z android OpenSLES)