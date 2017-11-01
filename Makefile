# 扩展
# ---------------------------------------------------------------------
EXTENSION=${EXT_NAME}.so
EXT_NAME=flame
EXT_VER=0.11.0
# PHP环境
# ---------------------------------------------------------------------
PHP_PREFIX?=/usr/local/php-7.0.19-test
PHP=${PHP_PREFIX}/bin/php
PHP_CONFIG=${PHP_PREFIX}/bin/php-config
# 编译参数
# ---------------------------------------------------------------------
CXX?=/usr/local/gcc-7.1.0/bin/g++
CXXFLAGS?= -g -O0
CXXFLAGS_CORE= -std=c++14 -fPIC
INCLUDES_CORE= `${PHP_CONFIG} --includes` -I./deps -I./deps/libuv/include -I./deps/mongo-c-driver/bin/include/libbson-1.0
# 链接参数
# ---------------------------------------------------------------------
LDFLAGS?=-Wl,-rpath=/usr/local/gcc-7.1.0/lib64/
LDFLAGS_CORE= -u get_module -Wl,-rpath='$$ORIGIN/'
LIBRARY=./deps/multipart-parser-c/multipart_parser.o \
 ./deps/buffer-queue/buffer_queue.o \
 ./deps/fastcgi-parser/fastcgi_parser.o \
 ./deps/kv-parser/kv_parser.o \
 ./deps/libphpext/libphpext.a \
 ./deps/libuv/.libs/libuv.a \
 ./deps/fmt/fmt/libfmt.a \
 ./deps/curl/lib/.libs/libcurl.a \
 ./deps/hiredis/libhiredis.a \
 ./deps/nghttp2/bin/lib/libnghttp2.a \
 ./deps/mongo-c-driver/bin/lib/libmongoc-1.0.a \
 ./deps/mongo-c-driver/bin/lib/libbson-1.0.a \
 ./deps/mongo-c-driver/bin/lib/libsnappy.a
# 代码和预编译头文件
# ---------------------------------------------------------------------
# SOURCES=$(shell find ./src -name "*.cpp")
SOURCES=$(shell find ./src/util -name "*.cpp") \
 ./src/extension.cpp \
 $(shell find ./src/flame -maxdepth 1 -name "*.cpp") \
 $(shell find ./src/flame/time -name "*.cpp") \
 $(shell find ./src/flame/os -name "*.cpp") \
 ./src/flame/db/db.cpp ./src/flame/db/redis.cpp \
 $(shell find ./src/flame/db/mongodb -name "*.cpp") \
 $(shell find ./src/flame/net -maxdepth 1 -name "*.cpp") \
 $(shell find ./src/flame/net/fastcgi -name "*.cpp") \
 $(shell find ./src/flame/net/http -name "*.cpp")
OBJECTS=$(SOURCES:%.cpp=%.o)
HEADERX=deps/deps.h.gch
# 扩展编译过程
# ----------------------------------------------------------------------
.PHONY: all install clean clean-deps clean-lnks update-deps test
all: ${EXTENSION}
update-deps:
	git submodule update --init
${EXTENSION}: ${LIBRARY} ${OBJECTS}
	${CXX} -shared ${OBJECTS} ${LIBRARY} ${LDFLAGS_CORE} ${LDFLAGS} -o $@
${HEADERX}: deps/deps.h
	${CXX} -x c++ ${CXXFLAGS_CORE} ${CXXFLAGS} ${INCLUDES_CORE} -c $^ -o $@
src/extension.o: src/extension.cpp
	${CXX} ${CXXFLAGS_CORE} -include ./deps/deps.h -DEXT_NAME=\"${EXT_NAME}\" -DEXT_VER=\"${EXT_VER}\" ${CXXFLAGS} ${INCLUDES_CORE} -c $^ -o $@
%.o: %.cpp ${HEADERX}
	${CXX} ${CXXFLAGS_CORE} -include ./deps/deps.h ${CXXFLAGS} ${INCLUDES_CORE} -c $< -o $@
# 清理安装
# ----------------------------------------------------------------------
clean:
	rm -f ${EXTENSION} ${OBJECTS} $(shell find ./src -name "*.o")
clean-lnks:
	find -type l | xargs rm
install: ${EXTENSION}
	rm -f `${PHP_CONFIG} --extension-dir`/${EXTENSION} && cp ${EXTENSION} `${PHP_CONFIG} --extension-dir`
# 依赖库的编译过程
# ----------------------------------------------------------------------
./deps/nghttp2/bin/lib/libnghttp2.a:
	cd ./deps/nghttp2; git submodule update --init; autoreconf -i; automake; autoconf; CFLAGS=-fPIC /bin/sh ./configure --disable-shared --prefix `pwd`/bin
	make -C ./deps/nghttp2 -j2 && make -C ./deps/nghttp2 install
	cd ./deps/nghttp2; find -type l | xargs rm
./deps/libphpext/libphpext.a:
	make -C ./deps/libphpext -j2
./deps/libuv/.libs/libuv.a:
	cd ./deps/libuv; /bin/sh ./autogen.sh; CFLAGS=-fPIC /bin/sh ./configure
	make -C ./deps/libuv -j2
	cd ./deps/libuv; find -type l | xargs rm
./deps/curl/lib/.libs/libcurl.a: ./deps/nghttp2/bin/lib/libnghttp2.a
	cd ./deps/curl; /bin/sh ./buildconf; PKG_CONFIG_PATH=../nghttp2/bin/lib CFLAGS=-fPIC ./configure --with-nghttp2=../nghttp2/bin
	make -C ./deps/curl -j2
	cd ./deps/curl; find -type l | xargs rm
./deps/hiredis/libhiredis.a:
	make -C ./deps/hiredis -j2
./deps/multipart-parser-c/multipart_parser.o:
	make -C ./deps/multipart-parser-c default
./deps/kv-parser/kv_parser.o:
	make -C ./deps/kv-parser all
./deps/fastcgi-parser/fastcgi_parser.o:
	make -C ./deps/fastcgi-parser all
./deps/buffer-queue/buffer_queue.o:
	make -C ./deps/buffer-queue all
./deps/mongo-c-driver/bin/lib/libmongoc-1.0.a: ./deps/mongo-c-driver/src/libbson/autogen.sh
	cd ./deps/mongo-c-driver; chmod +x ./autogen.sh; chmod +x ./src/libbson/autogen.sh;
	cd ./deps/mongo-c-driver/src/libbson; NOCONFIGURE=1 ./autogen.sh;
	cd ./deps/mongo-c-driver; NOCONFIGURE=1 ./autogen.sh;
	cd ./deps/mongo-c-driver; CFLAGS=-fPIC CXXFLAGS=-fPIC ./configure --prefix=`pwd`/bin --disable-automatic-init-and-cleanup --enable-static=yes --enable-shared=no
	make -C ./deps/mongo-c-driver -j2 && make -C ./deps/mongo-c-driver install
	cd ./deps/mongo-c-driver; find -type l | xargs rm
./deps/mongo-c-driver/src/libbson/autogen.sh:
	cd ./deps/mongo-c-driver; git submodule update --init;
	cd ./deps/mongo-c-driver/src/libbson; ln -s README.rst README
./deps/libev/.libs/libev.a:
	cd ./deps/libev; chmod +x ./autogen.sh; ./autogen.sh
	CFLAGS=-fPIC ./configure --disable-shared
	make -C ./deps/libev -j2
	cd ./deps/libev; find -type l | xargs rm
./deps/fmt/fmt/libfmt.a:
	cmake -DCMAKE_CXX_FLAGS=-fPIC -DCMAKE_C_FLAGS=-fPICCMAKE_CXX_FLAGS ./deps/fmt
	make -C ./deps/fmt -j2
# 依赖清理
# ---------------------------------------------------------------------
clean-deps:
	rm -f ${HEADERX}
	make -C ./deps/libphpext clean
	make -C ./deps/libuv clean
	make -C ./deps/hiredis clean
	make -C ./deps/curl clean
	make -C ./deps/nghttp2 clean
	rm -rf ./deps/nghttp2/bin
	make -C ./deps/mongo-c-driver clean
	rm -rf ./deps/mongo-c-driver/bin
