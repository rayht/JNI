cmake_minimum_required(VERSION 3.4.1)

add_library(
             native-lib
             SHARED
             src/main/cpp/native-lib.cpp)

include_directories(src/main/cpp/include)

#CMAKE_CXX_FLAGS 参数会传给C++的编译器
#如果是C则是CMAKE_C_FLAGS
#-L${CMAKE_SOURCE_DIR}/src/main/cpp/libs/${ANDROID_ABI}:编译的时候链接到这个目录下的SO。${ANDROID_ABI}：armeabi-v7a
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${CMAKE_SOURCE_DIR}/src/main/cpp/libs/${ANDROID_ABI}")

find_library(
              log-lib
              log )

target_link_libraries(
                       native-lib
                       ${log-lib} )