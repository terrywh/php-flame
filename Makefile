# 扩展
EXTENSION=${EXT_NAME}.so
EXT_NAME=flame
EXT_VER=0.7.0
# PHP环境
PHP_PREFIX=/usr/local/php-7.0.19-test
# PHP_PREFIX=/usr/local/php
PHP=${PHP_PREFIX}/bin/php
PHP_CONFIG=${PHP_PREFIX}/bin/php-config
# 编译参数
CXX?=/usr/local/gcc-7.1.0/bin/g++
CXXFLAGS?= -g -O0
CXXFLAGS_CORE= -std=c++14 -fPIC \
 -include ./deps/deps.h
INCLUDES_CORE= `${PHP_CONFIG} --includes`
# 链接参数
LDFLAGS?=-Wl,-rpath=/usr/local/gcc-7.1.0/lib64/
LDFLAGS_CORE= -u get_module -Wl,-rpath='$$ORIGIN/'
LIBRARY=./deps/libphpext/libphpext.a \
 ./deps/libuv/.libs/libuv.a
# 代码和预编译头文件
SOURCES=$(shell find ./src -name "*.cpp")
OBJECTS=$(SOURCES:%.cpp=%.o)
HEADERX=deps/deps.h.gch

.PHONY: all install clean clean-deps test

# 扩展编译过程
# ----------------------------------------------------------------------
all: ${EXTENSION}
${EXTENSION}: ${LIBRARY} ${HEADERX} ${OBJECTS}
	${CXX} -shared ${OBJECTS} ${LIBRARY} ${LDFLAGS_CORE} ${LDFLAGS} -o $@
${HEADERX}: deps/deps.h
	${CXX} -x c++ ${CXXFLAGS_CORE} ${CXXFLAGS} ${INCLUDES_CORE} -c $^ -o $@
src/extension.o: src/extension.cpp
	${CXX} ${CXXFLAGS_CORE} -DEXT_NAME=\"${EXT_NAME}\" -DEXT_VER=\"${EXT_VER}\" ${CXXFLAGS} ${INCLUDES_CORE} -c $^ -o $@
%.o: %.cpp
	${CXX} ${CXXFLAGS_CORE} ${CXXFLAGS} ${INCLUDES_CORE} -c $^ -o $@

clean:
	rm -f ${EXTENSION} ${OBJECTS} $(shell find ./src -name "*.o")
install: ${EXTENSION}
	cp -f ${EXTENSION} `${PHP_CONFIG} --extension-dir`
# 依赖库的编译过程
# ----------------------------------------------------------------------
./deps/libphpext/libphpext.a:
	make -C ./deps/libphpext
./deps/libuv/.libs/libuv.a:
	cd ./deps/libuv; ./autogen.sh; CFLAGS=-fPIC ./configure 
	make -C ./deps/libuv
# 依赖清理
# ---------------------------------------------------------------------
clean-deps:
	rm -f ${HEADERX}
	make -C ./deps/libphpext clean
	make -C ./deps/libuv clean
