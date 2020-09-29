cmake_minimum_required(VERSION 3.13)
# -----------------------------------------------------------------------------
# 环境配置
# -----------------------------------------------------------------------------
## 编译器
set(GCC_TOOLCHAIN /data/server/gcc CACHE PATH "Compiler: gcc compiler prefix")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED 17)
# set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(FLAME)
# -----------------------------------------------------------------------------
# 依赖项目
# -----------------------------------------------------------------------------
set(VENDOR_BOOST /data/vendor/boost-1.74 CACHE PATH "[dep] http://www.boost.org/")
set(VENDOR_FMT /data/vendor/fmt-7.0 CACHE PATH "[dep] https://fmt.dev/")
set(VENDOR_PARSER /data/vendor/parser-1.0)
set(VENDOR_LLTOML /data/vendor/lltoml-1.0)
set(VENDOR_HIREDIS /data/vendor/hiredis-0.14 CACHE PATH "[dep] https://github.com/redis/hiredis")
set(VENDOR_AMQP /data/vendor/amqpcpp-4.1 CACHE PATH "[dep] https://github.com/CopernicaMarketingSoftware/AMQP-CPP")
set(VENDOR_OPENSSL /data/vendor/openssl-1.1 CACHE PATH "[dep] https://www.openssl.org/")
set(VENDOR_MONGODB /data/vendor/mongoc-1.17 CACHE PATH "[dep] http://mongoc.org/")
set(VENDOR_RDKAFKA /data/vendor/rdkafka-1.5 CACHE PATH "[dep] https://github.com/edenhill/librdkafka")
set(VENDOR_PHP /data/server/php CACHE PATH "[dep] https://www.php.net/")
set(VENDOR_PHPEXT /data/vendor/phpext-4.0 CACHE PATH "[dep] https://github.com/terrywh/libphpext")
set(VENDOR_CARES /data/vendor/cares-1.16 CACHE PATH "[dep] https://c-ares.haxx.se/")
set(VENDOR_NGHTTP2 /data/vendor/nghttp2-1.41 CACHE PATH "[dep] https://nghttp2.org/")
set(VENDOR_CURL /data/vendor/curl-7.71 CACHE PATH "[dep] https://curl.haxx.se/")
set(VENDOR_MARIA /data/vendor/mariac-3.1 CACHE PATH "[dep] https://mariadb.com/downloads/#connectors")
## PHP 包含路径
execute_process(COMMAND ${VENDOR_PHP}/bin/php-config --includes
    COMMAND sed "s/ *-I/;/g"
    OUTPUT_VARIABLE VENDOR_PHP_INC_DIRS
    OUTPUT_STRIP_TRAILING_WHITESPACE)
## PHP 扩展路径
execute_process(COMMAND ${VENDOR_PHP}/bin/php-config --extension-dir
    OUTPUT_VARIABLE VENDOR_PHP_EXT_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE)
## PHP 头文件安装路径
execute_process(COMMAND ${VENDOR_PHP}/bin/php-config --include-dir
    OUTPUT_VARIABLE VENDOR_PHP_INC_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE)
## 安装
set(CMAKE_INSTALL_PREFIX ${VENDOR_PHP_EXT_DIR})
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY true)

# -----------------------------------------------------------------------------
# 项目
# -----------------------------------------------------------------------------
## CLANG 编译器（指定 GCC 工具链）
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options("--gcc-toolchain=${GCC_TOOLCHAIN}")
    add_link_options("--gcc-toolchain=${GCC_TOOLCHAIN}")
endif()
## 静态链接 C++ 库，方便部署
link_libraries(-static-libstdc++)
set(CMAKE_SHARED_LIBRARY_PREFIX "flame-")
execute_process(COMMAND find ${CMAKE_SOURCE_DIR}/src/core -name *.cpp
   COMMAND tr "\n" ";" OUTPUT_VARIABLE CORE_SOURCES)
add_library(core SHARED ${CORE_SOURCES})
execute_process(COMMAND find ${CMAKE_SOURCE_DIR}/src/http -name *.cpp
   COMMAND tr "\n" ";" OUTPUT_VARIABLE HTTP_SOURCES)
add_library(http SHARED ${HTTP_SOURCES})

# -----------------------------------------------------------------------------
# 编译选项
# -----------------------------------------------------------------------------
## core
# -----------------------------------------------------------------------------
set_property(TARGET core PROPERTY PUBLIC_HEADER
    src/core/core.h src/core/coroutine.h src/core/url.h src/core/util.h)
## core - 包含路径
target_include_directories(core 
    PUBLIC ${VENDOR_CURL}/include
    PUBLIC ${VENDOR_FMT}/include
    PUBLIC ${VENDOR_PHPEXT}/include
    PUBLIC ${VENDOR_BOOST}/include
)
target_include_directories(core SYSTEM
    PUBLIC ${VENDOR_PHP_INC_DIRS}
    PUBLIC ${VENDOR_OPENSSL}/include)
## core - 链接库
target_link_libraries(core
    ${VENDOR_PHPEXT}/lib/libphpext.a
    ${VENDOR_CURL}/lib/libcurl.a
    ${VENDOR_NGHTTP2}/lib/libnghttp2.a
    ${VENDOR_CARES}/lib/libcares.a
    ${VENDOR_FMT}/lib/libfmt.a
    ${VENDOR_OPENSSL}/lib/libssl.a
    ${VENDOR_OPENSSL}/lib/libcrypto.a
    pthread
)
## core - 安装
install(TARGETS core
    LIBRARY DESTINATION "."
    PUBLIC_HEADER DESTINATION "${VENDOR_PHP_INC_DIR}/ext/flame/core"
    OPTIONAL)

# http
# -----------------------------------------------------------------------------
target_include_directories(http
    PUBLIC ${VENDOR_CURL}/include
    PUBLIC ${VENDOR_FMT}/include
    PUBLIC ${VENDOR_PHPEXT}/include
    PUBLIC ${VENDOR_BOOST}/include
)
target_include_directories(http SYSTEM
    PUBLIC ${VENDOR_PHP_INC_DIRS}
    PUBLIC ${VENDOR_OPENSSL}/include)
target_link_libraries(http
    ${VENDOR_PHPEXT}/lib/libphpext.a
    ${VENDOR_CURL}/lib/libcurl.a
    ${VENDOR_NGHTTP2}/lib/libnghttp2.a
    ${VENDOR_CARES}/lib/libcares.a
    ${VENDOR_FMT}/lib/libfmt.a
    ${VENDOR_OPENSSL}/lib/libssl.a
    ${VENDOR_OPENSSL}/lib/libcrypto.a
    pthread
)

## http - 安装
install(TARGETS http
    LIBRARY DESTINATION "."
    PUBLIC_HEADER DESTINATION "${VENDOR_PHP_INC_DIR}/ext/flame/http"
    OPTIONAL)
# -----------------------------------------------------------------------------
# 预编译 （似乎 clangd 无法在存在 pch 的情况下正常工作）
# -----------------------------------------------------------------------------
# if (CMAKE_BUILD_TYPE MATCHES Release)
target_precompile_headers(core PUBLIC src/core/vendor.h)
target_precompile_headers(http PUBLIC src/http/vendor.h)
# endif()
