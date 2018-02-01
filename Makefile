# 扩展
# ---------------------------------------------------------------------
EXTENSION=${EXT_NAME}.so
EXT_NAME=flame
EXT_VER=1.3.0dev
# PHP环境
# ---------------------------------------------------------------------
PHP_PREFIX?=/usr/local/php
PHP=${PHP_PREFIX}/bin/php
PHP_CONFIG=${PHP_PREFIX}/bin/php-config
# 编译参数
# ---------------------------------------------------------------------
CXX?=/usr/local/gcc/bin/g++
CXXFLAGS?= -O2
LDFLAGS?=-Wl,-rpath=/usr/local/gcc/lib64/

CXXFLAGS_CORE= -std=c++11 -fPIC
LDFLAGS_CORE= -u get_module -Wl,-rpath='$$ORIGIN/'
INCLUDES_CORE= `${PHP_CONFIG} --includes` -I./deps -I./deps/libuv/include -I./deps/mongo-c-driver/bin/include/libbson-1.0 -I./deps/rabbitmq-c/librabbitmq -I./deps/mysql-connector-c/include
# 依赖库
# ---------------------------------------------------------------------
LIBRARY=./deps/multipart-parser-c/multipart_parser.o \
 ./deps/fastcgi-parser/fastcgi_parser.o \
 ./deps/kv-parser/kv_parser.o \
 ./deps/libphpext/libphpext.a \
 ./deps/libuv/.libs/libuv.a \
 ./deps/fmt/fmt/libfmt.a \
 ./deps/curl/lib/.libs/libcurl.a \
 ./deps/nghttp2/bin/lib/libnghttp2.a \
 ./deps/c-ares/bin/lib/libcares.a \
 ./deps/hiredis/libhiredis.a \
 ./deps/mongo-c-driver/bin/lib/libmongoc-1.0.a \
 ./deps/mongo-c-driver/bin/lib/libbson-1.0.a \
 ./deps/librdkafka/src/librdkafka.a \
 ./deps/http-parser/libhttp_parser.a \
 ./deps/rabbitmq-c/build/librabbitmq/librabbitmq.a \
 ./deps/mysql-connector-c/lib/libmysqlclient.a
# 代码和预编译头文件
# ---------------------------------------------------------------------
SOURCES=$(shell find ./src -name "*.cpp")
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
	${CXX} ${CXXFLAGS_CORE} -DEXT_NAME=\"${EXT_NAME}\" -DEXT_VER=\"${EXT_VER}\" ${CXXFLAGS} ${INCLUDES_CORE} -c $^ -o $@
%.o: %.cpp ${HEADERX}
	${CXX} ${CXXFLAGS_CORE} ${CXXFLAGS} ${INCLUDES_CORE} -c $< -o $@
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
	make -C ./deps/nghttp2
	make -C ./deps/nghttp2 install
	cd ./deps/nghttp2; find -type l | xargs rm
./deps/libphpext/libphpext.a:
	make -C ./deps/libphpext -j2
./deps/libuv/.libs/libuv.a:
	cd ./deps/libuv; /bin/sh ./autogen.sh; CFLAGS=-fPIC /bin/sh ./configure
	make -C ./deps/libuv -j2
	cd ./deps/libuv; find -type l | xargs rm
./deps/curl/lib/.libs/libcurl.a: ./deps/nghttp2/bin/lib/libnghttp2.a ./deps/c-ares/bin/lib/libcares.a
	cd ./deps/curl; /bin/sh ./buildconf; CFLAGS="-fPIC -I`pwd`/../nghttp2/bin/include -I`pwd`/../c-ares/bin/include" ./configure --with-nghttp2=`pwd`/../nghttp2/bin --enable-ares=`pwd`/../c-ares/bin --disable-shared
	make -C ./deps/curl -j2
	cd ./deps/curl; find -type l | xargs rm
./deps/hiredis/libhiredis.a:
	make -C ./deps/hiredis -j2
./deps/multipart-parser-c/multipart_parser.o:
	make -C ./deps/multipart-parser-c
./deps/kv-parser/kv_parser.o:
	make -C ./deps/kv-parser all
./deps/fastcgi-parser/fastcgi_parser.o:
	make -C ./deps/fastcgi-parser all
./deps/mongo-c-driver/bin/lib/libmongoc-1.0.a:
	cd ./deps/mongo-c-driver; git submodule update --init;
	cd ./deps/mongo-c-driver/src/libbson; rm -f README; ln -s README.rst README;
	cd ./deps/mongo-c-driver; chmod +x ./autogen.sh; chmod +x ./src/libbson/autogen.sh;
	cd ./deps/mongo-c-driver/src/libbson; NOCONFIGURE=1 ./autogen.sh;
	cd ./deps/mongo-c-driver; NOCONFIGURE=1 ./autogen.sh;
	cd ./deps/mongo-c-driver; CFLAGS=-fPIC CXXFLAGS=-fPIC ./configure --prefix=`pwd`/bin --disable-automatic-init-and-cleanup --disable-shm-counters --enable-static=yes --enable-shared=no
	make -C ./deps/mongo-c-driver
	make -C ./deps/mongo-c-driver install
	cd ./deps/mongo-c-driver; find -type l | xargs rm; 
./deps/fmt/fmt/libfmt.a:
	cd ./deps/fmt; cmake -DCMAKE_CXX_FLAGS=-fPIC -DCMAKE_C_FLAGS=-fPIC .
	make -C ./deps/fmt -j2
./deps/c-ares/bin/lib/libcares.a:
	cd ./deps/c-ares; chmod +x ./buildconf; ./buildconf;
	cd ./deps/c-ares; CFLAGS=-fPIC CPPFLAGS=-fPIC ./configure --prefix=`pwd`/bin
	make -C ./deps/c-ares -j2
	make -C ./deps/c-ares install
	cd ./deps/c-ares; find -type l | xargs rm; rm -f bin/lib/libcares.la; rm -f bin/lib/libcares.so*; rm -rf bin/lib/pkgconfig;
./deps/librdkafka/src/librdkafka.a:
	cd ./deps/librdkafka; chmod +x ./configure; chmod +x ./lds-gen.py; ./configure --enable-static
	make -C ./deps/librdkafka -j2
	cd ./deps/librdkafka; find -type l | xargs rm;
./deps/http-parser/libhttp_parser.a:
	cd ./deps/http-parser; CFLAGS="-fPIC" make package
./deps/rabbitmq-c/build/librabbitmq/librabbitmq.a:
	cd ./deps/rabbitmq-c; mkdir build; cd build;cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_C_FLAGS="-fPIC" ..
	cd ./deps/rabbitmq-c/build; CFLAGS="-fPIC" make

# 依赖清理
# ---------------------------------------------------------------------
clean-deps:
	rm -f ${HEADERX}
	make -C ./deps/nghttp2 clean
	rm -rf ./deps/nghttp2/bin
	make -C ./deps/libphpext clean
	make -C ./deps/libuv clean
	make -C ./deps/curl clean
	make -C ./deps/hiredis clean
	make -C ./deps/multipart-parser-c clean
	make -C ./deps/kv-parser clean
	make -C ./deps/fastcgi-parser clean
	make -C ./deps/mongo-c-driver clean
	rm -rf ./deps/mongo-c-driver/bin
	make -C ./deps/fmt clean
	make -C ./deps/c-ares clean
	rm -rf ./deps/c-ares/bin
	make -C ./deps/librdkafka clean
	make -C ./deps/http-parser clean
	rm -rf ./deps/rabbitmq-c/build
