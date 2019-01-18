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
./configure  --prefix=/data/vendor/php-7.2.13 --with-config-file-path=/data/vendor/php-7.2.13/etc --disable-fpm --disable-phar --disable-dom --disable-libxml --disable-simplexml --disable-xml --disable-xmlreader --disable-xmlwriter --with-openssl --with-readline --enable-mbstring --without-pear
```
#### Boost

``` Bash
./bootstrap.sh --prefix=/data/vendor/boost-1.69.0
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

#### mysql-connector-c v6.1.11
``` Bash
mv mysql-connector-c-6.1.11-src/ /data/vendor/mysqlc-6.1.11
rm /data/vendor/mysqlc-6.1.11/lib/*.so*
```

#### mongoc-driver
``` Bash
mkdir stage && cd stage
CC=gcc CXX=g++ cmake -DCMAKE_INSTALL_PREFIX=/data/vendor/mongoc-1.13.0 -DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS=-fPIC -DENABLE_STATIC=ON -DENABLE_SHM_COUNTERS=OFF -DENABLE_TESTS=OFF -DENABLE_EXAMPLES=OFF -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF ../
make
make install
rm /data/vendor/mongoc-1.13.0/lib/*.so*
```

#### AMQP-CPP
``` Bash
mkdir stage && cd stage
CC=gcc CXX=g++ cmake3 -DCMAKE_INSTALL_PREFIX=/data/vendor/amqpcpp-4.0.1 -DCMAKE_CXX_FLAGS=-fPIC -DCMAKE_BUILD_TYPE=Release -DAMQP-CPP_LINUX_TCP=on ../
make
make install
# rm /data/vendor/amqpcpp-4.0.1/lib/*.so*
```

#### Rdkafka
``` Bash
./configure --prefix=/data/vendor/rdkafka-0.11.6
make
make install
rm /data/vendor/rdkafka-0.11.6/lib/*.so*
```

#### HttpParser
``` Bash
mkdir -p /data/vendor/http-parser-2.8.1/lib
mkdir -p /data/vendor/http-parser-2.8.1/include
CFLAGS=-fPIC make libhttp_parser.o
cp libhttp_parser.o /data/vendor/http-parser-2.8.1/lib
cp http_parser.h /data/vendor/http-parser-2.8.1/include
```

#### HiRedis
``` Bash
make
PREFIX=/data/vendor/hiredis-0.14.0 make install
rm /data/vendor/hiredis-0.14.0/lib/*.so*
```
