# 扩展
# ---------------------------------------------------------------------
EXTENSION=${EXT_NAME}.so
EXT_NAME=flame
EXT_VER=2.0.2
# PHP环境
# ---------------------------------------------------------------------
PHP_PREFIX?=/usr/local/php
PHP=${PHP_PREFIX}/bin/php
PHP_CONFIG=${PHP_PREFIX}/bin/php-config
PHP_INCLUDES=$(shell ${PHP_CONFIG} --includes)
# 编译参数
# ---------------------------------------------------------------------
DEPS_DIR=$(shell pwd)/deps
CXX?=/usr/local/gcc/bin/g++
CXXFLAGS?= -O2
CXXFLAGS+= -std=c++11 -fPIC
LDFLAGS?=
LDFLAGS+= -u get_module -Wl,-rpath='$$ORIGIN/' -lcurl -lnghttp2 -lcares -lssl -lsasl2
INCLUDES= ${PHP_INCLUDES} -I./deps -I./deps/include -I./deps/include/libbson-1.0
# 依赖库
# ---------------------------------------------------------------------
LIBRARY= ./deps/libphpext/libphpext.a \
 ./deps/lib/libuv.a \
 ./deps/lib/libhttp_parser.a \
 ./deps/lib/libfmt.a \
 ./deps/lib/libhiredis.a \
 ./deps/lib/libmysqlclient.a \
 ./deps/lib/libmongoc-1.0.a \
 ./deps/lib/libbson-1.0.a \
 ./deps/lib/librdkafka.a \
 ./deps/lib/librabbitmq.a

# 代码和预编译头文件
# ---------------------------------------------------------------------
SOURCES=$(shell find ./src -name "*.cpp")
OBJECTS=$(SOURCES:%.cpp=%.o)
EXTERNAL_OBJECTS=./deps/multipart-parser-c/multipart_parser.o \
 ./deps/kv-parser/kv_parser.o \
 ./deps/fastcgi-parser/fastcgi_parser.o
HEADERX=deps/deps.h.gch
# 扩展编译过程
# ----------------------------------------------------------------------
.PHONY: all install clean clean-deps update-deps deps test
all: ${EXTENSION}

${EXTENSION}: ${LIBRARY} ${EXTERNAL_OBJECTS} ${OBJECTS}
	${CXX} -shared ${OBJECTS} ${EXTERNAL_OBJECTS} -Wl,--whole-archive ${LIBRARY} -Wl,--no-whole-archive ${LDFLAGS} -o $@
${HEADERX}: deps/deps.h
	${CXX} -x c++-header ${CXXFLAGS} ${INCLUDES} -c $^ -o $@ 
src/extension.o: src/extension.cpp
	${CXX} -DEXT_NAME=\"${EXT_NAME}\" -DEXT_VER=\"${EXT_VER}\" ${CXXFLAGS} ${INCLUDES} -c $^ -o $@ 
%.o: %.cpp ${HEADERX}
	${CXX} ${INCLUDES} ${CXXFLAGS} -c $< -o $@ 
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
./deps/multipart-parser-c/multipart_parser.o:
	make -C ./deps/multipart-parser-c
./deps/kv-parser/kv_parser.o:
	make -C ./deps/kv-parser all
./deps/fastcgi-parser/fastcgi_parser.o:
	make -C ./deps/fastcgi-parser all
./deps/libphpext/libphpext.a:
	make -C ./deps/libphpext -j2
./deps/lib/libuv.a:
	cd ./deps/libuv; /bin/sh ./autogen.sh; CFLAGS=-fPIC /bin/sh ./configure --prefix=${DEPS_DIR}
	make -C ./deps/libuv -j2
	make -C ./deps/libuv install
./deps/lib/libhttp_parser.a:
	CFLAGS="-fPIC" make -C ./deps/http-parser package
	cp ./deps/http-parser/libhttp_parser.a ${DEPS_DIR}/lib/
	cp ./deps/http-parser/http_parser.h ${DEPS_DIR}/include/
./deps/lib/libfmt.a:
	mkdir -p ./deps/fmt/build; cd ./deps/fmt/build; cmake -DCMAKE_INSTALL_PREFIX=${DEPS_DIR} -DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_CXX_FLAGS=-fPIC -DCMAKE_C_FLAGS=-fPIC ..
	make -C ./deps/fmt/build -j2
	make -C ./deps/fmt/build install
./deps/lib/libhiredis.a:
	make -C ./deps/hiredis -j2
	PREFIX=${DEPS_DIR} make -C ./deps/hiredis install
./deps/lib/libmysqlclient.a:
	cp -r ./deps/mysql-connector-c/include ./deps/include/mysql
	cp ./deps/mysql-connector-c/lib/libmysqlclient.a ./deps/lib/
./deps/lib/libmongoc-1.0.a:
	cd ./deps/mongo-c-driver/src/libbson; rm -f README; ln -s README.rst README;
	cd ./deps/mongo-c-driver; chmod +x ./autogen.sh; chmod +x ./src/libbson/autogen.sh;
	cd ./deps/mongo-c-driver/src/libbson; NOCONFIGURE=1 ./autogen.sh;
	cd ./deps/mongo-c-driver; NOCONFIGURE=1 ./autogen.sh; chmod +x ./configure;
	cd ./deps/mongo-c-driver; CFLAGS=-fPIC CXXFLAGS=-fPIC ./configure --prefix=${DEPS_DIR} --disable-automatic-init-and-cleanup --disable-shm-counters --enable-static=yes --enable-shared=no
	make -C ./deps/mongo-c-driver
	make -C ./deps/mongo-c-driver install
./deps/lib/librdkafka.a:
	cd ./deps/librdkafka; chmod +x ./configure; chmod +x ./lds-gen.py; ./configure --prefix=${DEPS_DIR} --enable-static
	make -C ./deps/librdkafka -j2
	make -C ./deps/librdkafka install
./deps/lib/librabbitmq.a:
	mkdir -p ./deps/rabbitmq-c/build; cd ./deps/rabbitmq-c/build; cmake -DCMAKE_INSTALL_PREFIX=${DEPS_DIR} -DCMAKE_INSTALL_LIBDIR=lib -DBUILD_SHARED_LIBS=OFF -DCMAKE_C_FLAGS="-fPIC" ..
	make -C ./deps/rabbitmq-c/build -j2
	make -C ./deps/rabbitmq-c/build install
# 依赖清理
# ---------------------------------------------------------------------
clean-deps:
	rm -f ${HEADERX}
	-make -C ./deps/multipart-parser-c clean
	-make -C ./deps/kv-parser clean
	-make -C ./deps/fastcgi-parser clean
	-make -C ./deps/libphpext clean
	-make -C ./deps/libuv clean
	-make -C ./deps/http-parser clean
	rm -rf ./deps/fmt/build
	-make -C ./deps/curl clean
	-make -C ./deps/nghttp2 clean
	-make -C ./deps/c-ares clean
	-make -C ./deps/hiredis clean
	-make -C ./deps/mongo-c-driver clean
	-make -C ./deps/librdkafka clean
	rm -rf ./deps/rabbitmq-c/build
	rm -rf ./deps/include/*
	rm -rf ./deps/lib/*
update-deps:
	git submodule update --init; 
	cd ./deps/mongo-c-driver; git submodule update --init; 
	cd ./deps/nghttp2; git submodule update --init; 
	cd ./deps/rabbitmq-c; rm -rf build; 
	mkdir -p ./deps/include
	mkdir -p ./deps/lib
deps: ${LIBRARY}
	
