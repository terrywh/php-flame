# 依赖
# ----------------------------------------------------------------------------------
VENDOR_PHP=/data/vendor/php-7.0.30
VENDOR_BOOST=/data/vendor/boost-1.67.0
VENDOR_PHPEXT=/data/vendor/phpext-1.0.0
VENDOR_PARSER=/data/vendor/parser-1.0.0
VENDOR_AMQP=/data/vendor/amqp-3.1.0
VENDOR_MYSQL=/data/vendor/mysql-connector-c-6.1.11
VENDOR_MONGODB=/data/vendor/mongoc-1.11.0

# 编译目标
# ---------------------------------------------------------------------------------
SOURCES=$(shell find ./src -name "*.cpp")
OBJECTS=$(SOURCES:%.cpp=%.o)
DEPENDS=$(SOURCES:%.cpp=%.d)
# 扩展
TARGETX=flame.so

# 编译选项
# ----------------------------------------------------------------------------------
# 包含路径
INCLUDES:= -I${VENDOR_PARSER}/include \
 -I${VENDOR_PHPEXT}/include \
 -isystem ${VENDOR_AMQP}/include \
 -isystem ${VENDOR_MONGODB}/include/libmongoc-1.0 -isystem ${VENDOR_MONGODB}/include/libbson-1.0 \
 -isystem ${VENDOR_MYSQL}/include \
 -isystem ${VENDOR_BOOST}/include \
 $(shell ${VENDOR_PHP}/bin/php-config --includes | sed 's/-I/-isystem/g')
CXX=clang++
CXXFLAGS?= -g -O0
CXXFLAGS+= -std=c++11 -fPIC ${INCLUDES}
COMPILER:=$(shell ${CXX} --version | head -n1 | awk '{print $$1}')

# 预编译头
ifeq '${COMPILER}' 'clang'
PCH=pch
else
PCH=gch
endif
PCHEADER=./src/vendor.h

# 连接选项
# ---------------------------------------------------------------------------------
# 依赖库
LIBRARY+= -L${VENDOR_MYSQL}/lib -lmysqlclient \
 -L${VENDOR_AMQP}/lib -lamqpcpp \
 -L${VENDOR_MONGODB}/lib -lmongoc-static-1.0 -lbson-static-1.0 \
 -L${VENDOR_PHPEXT}/lib -lphpext \
 -L${VENDOR_BOOST}/lib -lboost_system -lboost_thread -lboost_filesystem \
 -lssl -lcrypto -lsasl2 -lpthread -lrt

LDFLAGS?=
LDFLAGS+= -Wl,-rpath='$$ORIGIN/' ${LIBRARY}

# 编译过程
# ---------------------------------------------------------------------------------
# 虚拟目标
.PHONY: all clean install var
all: ${TARGETX}

clean:
	rm -f ${TARGETX} ${PCHEADER}.${PCH} ${OBJECTS} `find ./ -type f \( -name "*.o" -o -name "*.d" \)`
install: EXTENSION_DIR:=$(shell ${VENDOR_PHP}/bin/php-config --extension-dir)
install: ${TARGETX}
	sudo rm -f ${EXTENSION_DIR}/${TARGETX}
	sudo cp ${TARGETX} ${EXTENSION_DIR}/
var:
	@echo ${INCLUDES}
	@echo ${COMPILER}
	@echo ${PCH}

# 依赖定义
-include ${DEPENDS} ${PCHEADER}.d

${PCHEADER}.d:
${PCHEADER}.${PCH}: ${PCHEADER}
	${CXX} ${CXXFLAGS} -x c++-header -c ${PCHEADER} -MMD -MP -o $@
%.o: %.cpp ${PCHEADER}.${PCH}
	${CXX} ${CXXFLAGS} -include ${PCHEADER} -MMD -MP -c $< -o $@


# 目标链接
${TARGETX}: ${TARGET_PCH} ${OBJECTS}
	${CXX} -shared ${OBJECTS} ${LDFLAGS} -o $@