cmake_minimum_required(VERSION 3.10)
# 默认编译器
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif(NOT CMAKE_BUILD_TYPE)
if(NOT CMAKE_C_COMPILER)
    set(CMAKE_C_COMPILER /data/vendor/llvm-7.0.0/bin/clang)
endif()
if(NOT CMAKE_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER /data/vendor/llvm-7.0.0/bin/clang++)
endif()
project(FLAME)
# 项目源文件
execute_process(COMMAND find ${CMAKE_SOURCE_DIR}/src -name *.cpp
    COMMAND tr "\n" ";"
    OUTPUT_VARIABLE SOURCES)
add_library(FLAME SHARED ${SOURCES})
# 依赖项目录
set(VENDOR_GCC /data/vendor/gcc-8.2.0)
set(VENDOR_LLVM /data/vendor/llvm-7.0.0)
set(VENDOR_PHP /data/vendor/php-7.2.13)
execute_process(COMMAND ${VENDOR_PHP}/bin/php-config --includes
    COMMAND sed "s/ *-I/;/g"
    OUTPUT_VARIABLE VENDOR_PHP_INCLUDES
    OUTPUT_STRIP_TRAILING_WHITESPACE)
set(VENDOR_PHPEXT /data/vendor/phpext-1.1.1)
set(VENDOR_PARSER /data/vendor/parser-1.0.0)
set(VENDOR_BOOST /data/vendor/boost-1.69.0)
set(VENDOR_MYSQL /data/vendor/mysqlc-6.1.11)
set(VENDOR_HIREDIS /data/vendor/hiredis-0.14.0)
set(VENDOR_MONGODB /data/vendor/mongoc-1.13.0)
set(VENDOR_AMQP /data/vendor/amqpcpp-4.0.1)
set(VENDOR_RDKAFKA /data/vendor/rdkafka-0.11.6)
set(VENDOR_HTTPPARSER /data/vendor/http-parser-2.8.1)
# 编译选项
set(CUSTOM_COMPILE_FLAGS "-std=c++17")
set(CUSTOM_LINK_FLAGS "-static-libstdc++")
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CUSTOM_COMPILE_FLAGS "${CUSTOM_COMPILE_FLAGS} --gcc-toolchain=${VENDOR_GCC}")
    set(CUSTOM_LINK_FLAGS "${CUSTOM_LINK_FLAGS} --gcc-toolchain=${VENDOR_GCC}")
endif()
set_target_properties(FLAME PROPERTIES
    COMPILE_FLAGS "${CUSTOM_COMPILE_FLAGS}"
    LINK_FLAGS "${CUSTOM_LINK_FLAGS}"
    PREFIX ""
    OUTPUT_NAME "flame")
# 包含路径
target_include_directories(FLAME SYSTEM PRIVATE
    ${VENDOR_PHP_INCLUDES}
    ${VENDOR_BOOST}/include
    ${VENDOR_PHPEXT}/include
    ${VENDOR_PARSER}/include
    ${VENDOR_MYSQL}/include
    ${VENDOR_HIREDIS}/include
    ${VENDOR_MONGODB}/include/libmongoc-1.0
    ${VENDOR_MONGODB}/include/libbson-1.0
    ${VENDOR_AMQP}/include
    ${VENDOR_RDKAFKA}/include
    ${VENDOR_HTTPPARSER}/include
)
# 链接库
target_link_libraries(FLAME
    ${VENDOR_HTTPPARSER}/lib/libhttp_parser.o
    ${VENDOR_PHPEXT}/lib/libphpext.a
    ${VENDOR_MYSQL}/lib/libmysqlclient.a
    ${VENDOR_HIREDIS}/lib/libhiredis.a
    ${VENDOR_MONGODB}/lib/libmongoc-static-1.0.a
    ${VENDOR_MONGODB}/lib/libbson-static-1.0.a
    ${VENDOR_AMQP}/lib/libamqpcpp.a
    ${VENDOR_RDKAFKA}/lib/librdkafka.a
    ${VENDOR_BOOST}/lib/libboost_program_options.a
    ${VENDOR_BOOST}/lib/libboost_context.a
    ${VENDOR_BOOST}/lib/libboost_system.a
    ${VENDOR_BOOST}/lib/libboost_thread.a
    ${VENDOR_BOOST}/lib/libboost_filesystem.a
    sasl2
    ssl
    crypto
    pthread
    rt
)
