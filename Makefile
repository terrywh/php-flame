LIBEXTPHP_ROOT=../libphpext

EXT_NAME=flame
EXT_VER=0.7.0

PHP_PREFIX=/usr/local/php-7.0.19-test
# PHP_PREFIX=/usr/local/php
PHP=${PHP_PREFIX}/bin/php
PHP_CONFIG=${PHP_PREFIX}/bin/php-config

CXX?=g++
CXXFLAGS?= -g -O0
LDFLAGS?=-Wl,-rpath=/usr/local/gcc7/lib64/
LDFLAGS_DEFAULT= -u get_module -Wl,-rpath='$$ORIGIN/'

INCLUDE= `${PHP_CONFIG} --includes` \
 -I${LIBEXTPHP_ROOT} \
 -I/data/vendor/libuv/include
CXXFLAGS_DEFAULT= -std=c++11 -fPIC \
 -include ./deps/deps.h

LIBRARY=${LIBEXTPHP_ROOT}/libphpext.a \
 /data/vendor/libuv/lib/libuv.a

SOURCES=$(shell find ./src -name "*.cpp")
OBJECTS=$(SOURCES:%.cpp=%.o)
HEADERX=deps/deps.h.gch

EXTENSION=${EXT_NAME}.so

.PHONY: all install clean

all: ${EXTENSION}

${EXTENSION}: ${HEADERX} ${OBJECTS}
	${CXX} -shared ${OBJECTS} ${LIBRARY} ${LDFLAGS_DEFAULT} ${LDFLAGS} -o $@
${HEADERX}: deps/deps.h
	${CXX} -x c++ ${CXXFLAGS_DEFAULT} ${CXXFLAGS} ${INCLUDE} -c $^ -o $@
src/extension.o: src/extension.cpp
	${CXX} ${CXXFLAGS_DEFAULT} -DEXT_NAME=\"${EXT_NAME}\" -DEXT_VER=\"${EXT_VER}\" ${CXXFLAGS} ${INCLUDE} -c $^ -o $@
%.o: %.cpp
	${CXX} ${CXXFLAGS_DEFAULT} ${CXXFLAGS} ${INCLUDE} -c $^ -o $@

clean:
	rm -f ${HEADERX}
	rm -f ${EXTENSION} ${OBJECTS} $(shell find ./src -name "*.o")
install: ${EXTENSION}
	cp -f ${EXTENSION} `${PHP_CONFIG} --extension-dir`

test:
