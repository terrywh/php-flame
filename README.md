### 功能项

* **core** - 核心，框架初始化设置，协程启动，协程队列，协程锁等；
* **time** - 时间相关，协程休眠，当前时间等；
* **log** - 简单日志记录功能，多进程支持，支持日志登记过滤设置；
* **os** - 操作系统相关信息获取，异步进程启停操作等；
* **tcp** - 封装 TCP 相关服务端客户端功能；
* **http** - 简单的 HTTP 客户端，支持长连 Keep-Alive 及相关连接数控制；HTTP 服务端，简单 PATH 处理器；（暂不支持 HTTPS 相关）
* **mysql** - 简单 MySQL 客户端（连接池），提供部分简化方法，如 `insert/delete/one` 等；
* **redis** - 简单 Redis 客户端（连接池）；
* **mongodb** - 简单 MongoDB 客户端（连接池），提供部分简化方法，如 `insert/delete/one/count` 等；
* **rabbitmq** - 简单 RabbitMQ 生产消费支持；
* **kafka** - 简单 Kafka 生产消费支持（仅支持 Kafka 0.10+ 群组消费）；

请参看 `/doc` 目录下相关 PHP 文件的文档注释，或将该目录挂接再 IDE 内提供**自动完成**；

### 常见问题

https://github.com/terrywh/php-flame/wiki/%E5%B8%B8%E8%A7%81%E9%97%AE%E9%A2%98

### 依赖库

#### PHP
``` Bash
./configure --prefix=/data/vendor/php-7.2.15 --with-config-file-path=/data/vendor/php-7.2.15/etc --disable-fpm --disable-phar --disable-dom --disable-libxml --disable-simplexml --disable-xml --disable-xmlreader --disable-xmlwriter --with-openssl --with-readline --enable-mbstring --without-pear --with-curl --enable-mbstring --host=x86_64-linux-gnu --target=x86_64-linux-gnu
```
#### Boost

``` Bash
./bootstrap.sh --prefix=/data/vendor/boost-1.69.0~/
./b2 -j4 --prefix=/data/vendor/boost-1.69.0 cxxflags="-fPIC" variant=release link=static threading=multi install
```

#### libphpext
``` Bash
CXXFLAGS="-O2" make -j4
make install
```

#### cpp-parser
``` Bash
make install
```

#### mysql-connector-c v8.0.15
``` Bash
mkdir -p /data/vendor/mysqlc-8.0.15/lib
cp -R mysql-8.0.15-linux-glibc2.12-x86_64/include /data/vendor/mysqlc-8.0.15
cp mysql-8.0.15-linux-glibc2.12-x86_64/lib/libmysqlclient.a /data/vendor/mysqlc-8.0.15/lib
```

#### mongoc-driver
``` Bash
mkdir stage && cd stage
CC=gcc CXX=g++ cmake -DCMAKE_INSTALL_PREFIX=/data/vendor/mongoc-1.14.0 -DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS=-fPIC -DENABLE_STATIC=ON -DENABLE_SHM_COUNTERS=OFF -DENABLE_TESTS=OFF -DENABLE_EXAMPLES=OFF -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF ../
# with openssl 1.0 in another path
# -DOPENSSL_INCLUDE_DIR=/usr/include/openssl-1.0 -DOPENSSL_SSL_LIBRARY=/usr/lib/openssl-1.0/libssl.so -DOPENSSL_CRYPTO_LIBRARY=/usr/lib/openssl-1.0/libcrypto.so
make
make install
rm /data/vendor/mongoc-1.14.0/lib/*.so*
```

#### AMQP-CPP
``` Bash
mkdir stage && cd stage
CC=gcc CXX=g++ cmake -DCMAKE_INSTALL_PREFIX=/data/vendor/amqpcpp-4.1.3 -DCMAKE_CXX_FLAGS=-fPIC -DCMAKE_BUILD_TYPE=Release -DAMQP-CPP_LINUX_TCP=on ../
make
make install
```

#### Rdkafka
``` Bash
./configure --prefix=/data/vendor/rdkafka-1.0.0
make
make install
rm /data/vendor/rdkafka-1.0.0/lib/*.so*
```

#### HttpParser
``` Bash
mkdir -p /data/vendor/http-parser-2.9.0/lib
mkdir -p /data/vendor/http-parser-2.9.0/include
CC=gcc CFLAGS=-fPIC make libhttp_parser.o
cp libhttp_parser.o /data/vendor/http-parser-2.9.0/lib
cp http_parser.h /data/vendor/http-parser-2.9.0/include
```

#### HiRedis
``` Bash
CC=gcc make
PREFIX=/data/vendor/hiredis-0.14.0 make install
rm /data/vendor/hiredis-0.14.0/lib/*.so*
```
