cmake_minimum_required(VERSION 3.13)
# -----------------------------------------------------------------------------
# 默认编译器
# -----------------------------------------------------------------------------
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif(NOT CMAKE_BUILD_TYPE)
set(CLANG_GCC_TOOLCHAIN "<default>" CACHE PATH "gcc toolchain install prefix, used with clang --gcc-toolchain option")
if (NOT (CLANG_GCC_TOOLCHAIN STREQUAL "<default>"))
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --gcc-toolchain=${CLANG_GCC_TOOLCHAIN}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --gcc-toolchain=${CLANG_GCC_TOOLCHAIN}")
endif()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# -----------------------------------------------------------------------------
# 项目源文件
# -----------------------------------------------------------------------------
project(FLAME)
execute_process(COMMAND find ${CMAKE_SOURCE_DIR}/src -name *.cpp
    COMMAND tr "\n" ";"
    OUTPUT_VARIABLE SOURCES)
add_library(FLAME SHARED ${SOURCES})
# -----------------------------------------------------------------------------
# 依赖项目录
# -----------------------------------------------------------------------------
set(VENDOR_BOOST /data/vendor/boost-1.71.0)
set(VENDOR_PARSER /data/vendor/parser-1.0.0)
set(VENDOR_LLTOML /data/vendor/lltoml-1.0.2)
set(VENDOR_HIREDIS /data/vendor/hiredis-0.14.0)
set(VENDOR_AMQP /data/vendor/amqpcpp-4.1.5)
set(VENDOR_OPENSSL /data/vendor/openssl-1.1.1d)
set(VENDOR_MONGODB /data/vendor/mongoc-1.15.1)
set(VENDOR_RDKAFKA /data/vendor/rdkafka-1.2.1)
set(VENDOR_PHP /data/server/php-7.2.22)
set(VENDOR_PHPEXT /data/vendor/phpext-3.0.1)
set(VENDOR_CARES /data/vendor/cares-1.15.0)
set(VENDOR_NGHTTP2 /data/vendor/nghttp2-1.39.2)
set(VENDOR_CURL /data/vendor/curl-7.65.3)
set(VENDOR_MARIA /data/vendor/mariac-3.1.4)

execute_process(COMMAND ${VENDOR_PHP}/bin/php-config --includes
    COMMAND sed "s/ *-I/;/g"
    OUTPUT_VARIABLE VENDOR_PHP_INCLUDES
    OUTPUT_STRIP_TRAILING_WHITESPACE)

# -----------------------------------------------------------------------------
# 编译选项
# -----------------------------------------------------------------------------
target_compile_options(FLAME BEFORE
	PRIVATE "-std=c++17"
    PRIVATE "-pthread")
target_link_options(FLAME BEFORE
	PRIVATE "-static-libstdc++"
    PRIVATE "-pthread")
set_target_properties(FLAME PROPERTIES
    PREFIX ""
    OUTPUT_NAME "flame")
# 包含路径
target_include_directories(FLAME SYSTEM PRIVATE
    ${VENDOR_PARSER}/include
    ${VENDOR_LLTOML}/include
    ${VENDOR_HIREDIS}/include
    ${VENDOR_AMQP}/include
    ${VENDOR_RDKAFKA}/include
    ${VENDOR_MONGODB}/include/libmongoc-1.0
    ${VENDOR_MONGODB}/include/libbson-1.0
    ${VENDOR_MARIA}/include 
    ${VENDOR_NGHTTP2}/include
    ${VENDOR_CURL}/include
    ${VENDOR_PHPEXT}/include
    ${VENDOR_PHP_INCLUDES}
    ${VENDOR_OPENSSL}/include
    ${VENDOR_BOOST}/include
)
# 链接库
target_link_libraries(FLAME
    ${VENDOR_HIREDIS}/lib/libhiredis.a
    ${VENDOR_LLTOML}/lib/liblltoml.a
    ${VENDOR_PHPEXT}/lib/libphpext.a
    ${VENDOR_MARIA}/lib/mariadb/libmariadbclient.a
    ${VENDOR_MONGODB}/lib/libmongoc-static-1.0.a
    ${VENDOR_MONGODB}/lib/libbson-static-1.0.a
    ${VENDOR_AMQP}/lib/libamqpcpp.a
    ${VENDOR_RDKAFKA}/lib/librdkafka.a
    ${VENDOR_BOOST}/lib/libboost_program_options.a
    ${VENDOR_BOOST}/lib/libboost_context.a
    ${VENDOR_BOOST}/lib/libboost_system.a
    ${VENDOR_BOOST}/lib/libboost_thread.a
    ${VENDOR_BOOST}/lib/libboost_filesystem.a
    ${VENDOR_CURL}/lib/libcurl.a
    ${VENDOR_NGHTTP2}/lib/libnghttp2.a
    ${VENDOR_CARES}/lib/libcares.a
    ${VENDOR_OPENSSL}/lib/libssl.a
    ${VENDOR_OPENSSL}/lib/libcrypto.a
    resolv
    z
    dl
    m
    nsl
    rt
)
