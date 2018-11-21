cmake_minimum_required(VERSION 3.10)

# 依赖项目录
set(VENDOR_GCC /data/vendor/gcc-8.2.0)
set(VENDOR_LLVM /data/vendor/llvm-7.0.0)
set(VENDOR_PHP /data/vendor/php-7.2.11)
execute_process(COMMAND ${VENDOR_PHP}/bin/php-config --includes
    COMMAND sed "s/ *-I/;/g"
    OUTPUT_VARIABLE VENDOR_PHP_INCLUDES
    OUTPUT_STRIP_TRAILING_WHITESPACE)
set(VENDOR_PHPEXT /data/vendor/phpext-1.1.0)
set(VENDOR_PARSER /data/vendor/parser-1.0.0)
set(VENDOR_BOOST /data/vendor/boost-1.68.0)
set(VENDOR_MYSQL /data/vendor/mysqlc-6.1.11)
set(VENDOR_MONGODB /data/vendor/mongoc-1.13.0)
set(VENDOR_AMQP /data/vendor/amqpcpp-4.0.0)
set(VENDOR_RDKAFKA /data/vendor/rdkafka-0.11.6)
# 基本参数
set(CMAKE_BUILD_TYPE "Debug")
# set(CMAKE_C_COMPILER /data/vendor/llvm-7.0.0/bin/clang)
# set(CMAKE_CXX_COMPILER /data/vendor/llvm-7.0.0/bin/clang++)
set(CMAKE_C_COMPILER /data/vendor/gcc-8.2.0/bin/gcc)
set(CMAKE_CXX_COMPILER /data/vendor/gcc-8.2.0/bin/g++)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
# 项目
project(FLAME)
execute_process(COMMAND find ${CMAKE_SOURCE_DIR}/src -name *.cpp
    COMMAND tr "\n" ";"
    OUTPUT_VARIABLE SOURCES)
add_library(FLAME SHARED ${SOURCES})
set_target_properties(FLAME PROPERTIES
#   COMPILE_FLAGS "--gcc-toolchain=${VENDOR_GCC} -std=c++17"
   COMPILE_FLAGS "-std=c++17"
   LINK_FLAGS "-static-libstdc++"
   PREFIX ""
   OUTPUT_NAME "flame")
# 包含路径
target_include_directories(FLAME SYSTEM PRIVATE
    ${VENDOR_PHP_INCLUDES}
    ${VENDOR_BOOST}/include
    ${VENDOR_PHPEXT}/include
    ${VENDOR_PARSER}/include
    ${VENDOR_MYSQL}/include
    ${VENDOR_MONGODB}/include/libmongoc-1.0
    ${VENDOR_MONGODB}/include/libbson-1.0
    ${VENDOR_AMQP}/include
    ${VENDOR_RDKAFKA}/include
)
# 链接库
target_link_libraries(FLAME
    ${VENDOR_PHPEXT}/lib/libphpext.a
    ${VENDOR_MYSQL}/lib/libmysqlclient.a
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