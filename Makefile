LIBEXTPHP_ROOT=../libphpext

EXT_NAME=flame
EXT_VER=0.7.0

PHP_PREFIX=/usr/local/php-7.0.19-test
# PHP_PREFIX=/usr/local/php

CXX=/usr/local/gcc-7.1.0/bin/g++
CXXFLAGS?= -g -O0
LDFLAGS=-Wl,-rpath=/usr/local/gcc-7.1.0/lib64/

PHP=${PHP_PREFIX}/bin/php
PHP_CONFIG=${PHP_PREFIX}/bin/php-config
LDFLAGS_DEFAULT= -u get_module -Wl,-rpath='$$ORIGIN/'
CXXFLAGS_DEFAULT= -std=c++14 -fPIC \
 -include ./deps.h
INCLUDE= `${PHP_CONFIG} --includes` \
 -I${LIBEXTPHP_ROOT} \
 -I/data/vendor/libuv/include \
 -I/data/vendor/hiredis \
 -I/data/vendor/libcurl/include
LIBRARY=${LIBEXTPHP_ROOT}/libphpext.a \
 /data/vendor/libuv/lib/libuv.a \
 /data/vendor/hiredis/lib/libhiredis.a \
 /data/vendor/libcurl/lib/libcurl.a -lrt

SOURCES=$(shell find ./src -name "*.cpp")
OBJECTS=$(SOURCES:%.cpp=%.o)
HEADERX=deps.h.gch

EXTENSION=${EXT_NAME}.so

.PHONY: all install clean clean-deps test

all: ${EXTENSION}

${EXTENSION}: ${HEADERX} ${OBJECTS}
	${CXX} -shared ${OBJECTS} ${LIBRARY} ${LDFLAGS_DEFAULT} ${LDFLAGS} -o $@
${HEADERX}: deps.h
	${CXX} -x c++ ${CXXFLAGS_DEFAULT} ${CXXFLAGS} ${INCLUDE} -c $^ -o $@
src/extension.o: src/extension.cpp
	${CXX} ${CXXFLAGS_DEFAULT} -DEXT_NAME=\"${EXT_NAME}\" -DEXT_VER=\"${EXT_VER}\" ${CXXFLAGS} ${INCLUDE} -c $^ -o $@
%.o: %.cpp
	${CXX} ${CXXFLAGS_DEFAULT} ${CXXFLAGS} ${INCLUDE} -c $^ -o $@

clean-deps:
	rm -f ${HEADERX}
clean:
	rm -f ${EXTENSION} ${OBJECTS} $(shell find ./src -name "*.o")
install: ${EXTENSION}
	cp -f ${EXTENSION} `${PHP_CONFIG} --extension-dir`
test:
