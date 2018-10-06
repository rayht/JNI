
cmake_minimum_required(VERSION 3.4.1)

# 引入指定目录下的CMakeLists.txt
add_subdirectory(src/main/cpp/librtmp)

file(GLOB SOURCE src/main/cpp/*.cpp )

add_library(
             native-lib
             SHARED
             ${SOURCE} )

include_directories(/src/main/cpp/include)

find_library(
              log-lib
              log )

#CMAKE_CXX_FLAGS 参数会传给C++的编译器
#如果是C则是CMAKE_C_FLAGS
#CMAKE_SOURCE_DIR绝对路径
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${CMAKE_SOURCE_DIR}/src/main/cpp/libs/${ANDROID_ABI}")

target_link_libraries(
                       native-lib
                       rtmp
                       x264
                       ${log-lib} )