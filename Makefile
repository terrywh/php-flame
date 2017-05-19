#ROOT_TERRYWH=/data/wuhao/cpdocs/github.com/terrywh
ROOT_TERRYWH=..

EXTENSION_NAME=flame
EXTENSION_VERSION=0.3.0

ROOT_PROJECT=${ROOT_TERRYWH}/php-${EXTENSION_NAME}

# PHP_PREFIX=/usr/local/php-7.0.19-test
PHP_PREFIX=/usr/local/php
PHP=${PHP_PREFIX}/bin/php
PHP_CONFIG=${PHP_PREFIX}/bin/php-config

CXX?=g++
CXXFLAGS?= -g -O0
INCLUDE= -I${ROOT_TERRYWH}/libphpext -I/data/vendor/libevent/include `${PHP_CONFIG} --includes`
LIBRARY= ${ROOT_TERRYWH}/libphpext/libphpext.a /data/vendor/libevent/lib/libevent.a /data/vendor/libevent/lib/libevent_pthreads.a -lpthread

SOURCES=$(shell find ./src -name "*.cpp")
OBJECTS=$(SOURCES:%.cpp=%.o)

EXTENSION=${EXTENSION_NAME}.so

.PHONY: install clean

${EXTENSION}: ${OBJECTS} ${ROOT_TERRYWH}/libphpext/libphpext.a
	${CXX} -shared ${OBJECTS} ${LIBRARY} -Wl,-rpath='$$ORIGIN/' -Wl,-rpath='/usr/local/gcc6/lib64/' -o ${EXTENSION_NAME}.so
%.o: %.cpp
	${CXX} -std=c++11 -fPIC -DEXTENSION_NAME=\"${EXTENSION_NAME}\" -DEXTENSION_VERSION=\"${EXTENSION_VERSION}\" ${CXXFLAGS} ${INCLUDE} -c $^ -o $@

clean:
	rm -f ${EXTENSION} ${OBJECTS} $(shell find ./src -name "*.o")
install: ${EXTENSION}
	cp -f ${EXTENSION} `${PHP_CONFIG} --extension-dir`

test:
