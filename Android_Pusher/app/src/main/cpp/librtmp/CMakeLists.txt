

cmake_minimum_required(VERSION 3.4.1)

#取消NO_CRYPTO宏定义
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNO_CRYPTO")

#file(GLOB rtmp_source *.c)

add_library(
             rtmp

             STATIC

            amf.c
             hashswf.c
             log.c
             parseurl.c
             rtmp.c)