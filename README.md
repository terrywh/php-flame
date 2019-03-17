### FLAME
最初开发 FLAME 是为了给咱 PHP 争口气，开发者不能总是因为 “并行处理能力不足” 被各方高玩鄙视；最初试用了 [Swoole](https://www.swoole.com/) 但被其进程模型搞得一头雾水，而且可能也是因为早期版本，还出现了各式各样的问题；于是在各方支持下，自行开发了 FLAME 框架；

FLAME 能够在很大程度上提高 PHP 处理并行能力, 其定位在于简化开发常见互联网业务高性能服务程序的难度和代价：
* 网络底层异步化；
* 同步驱动连接池挂接线程池；
* 使用 PHP 内置协程 (Generator) 机制；
* 对等多进程工作模式；

陆续接入了 MySQL / Kafka / Redis / MongoDB / RabbitMQ 等第三方驱动，满足日常大多数接口型开发需求；

为了简化框架的学习成本、使用难度，在后期使用内置协程机制代替 `yield` 显式协程，并补充完善了很多功能：
* 协程队列，协程管道；
* 协程、异步错误捕获；
* 异步进程；网卡地址读取；毫秒、标准时间；...
* 平滑起停控制；
* 多进程日志、重载；
* ...

在适配到 PHP 7.2 版本以后，陆陆续续已经使用在了公司内各种业务线；在稳定性方面已经有了一定的底子；

当然说的很高大上、白富美，但实在能力有限，同时各种业务压力，没有时间详尽的测试整个框架的各项组合功能，框架可能存在各类问题、缺陷；迫切期望大家能帮忙发现和完善，在此感谢~ 大礼参拜~

以下代码实现了几个简单的 HTTP 接口：
``` PHP
flame\init("http_server_demo");
flame\go(function() {
    $server = new flame\http\server(":::56101");
    $server->before(function($req, $res) {
            $req->data["before"] = flame\time\now();
        })->get("/hello", function($req, $res) {
            $res->body = "world";
        })->post("/path", function($req, $res) {
            $res->write("CHUNKED RESPONSE:")
            $res->write($res->body);
            $res->end();
        })->after(function($req, $res, $r) {
            if(!$r) {
                $res->status = 404;
                $res->file(__DIR__."/404.html");
            }
        });
    $server->run();
});
flame\run();
```

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
* **hash** - 提供了若干哈希算法, 例如 murmur2 / crc64 / xxhash64 等；
* **encoding** - 提供了若干编码、序列化函数，例如 bson 等；
* **compress** - 提供了若干压缩算法，例如 snappy 等；

请参看 `/doc` `/test` 目录下相关 PHP 文件的文档注释；也可将 `/doc` 挂接在 IDE 内方便提供 **自动完成** 等；

### 常见问题

https://github.com/terrywh/php-flame/wiki/%E5%B8%B8%E8%A7%81%E9%97%AE%E9%A2%98

### 依赖库

#### PHP
``` Bash
./configure --prefix=/data/vendor/php-7.2.15 --with-config-file-path=/data/vendor/php-7.2.15/etc --disable-fpm --disable-phar --disable-dom --disable-libxml --disable-simplexml --disable-xml --disable-xmlreader --disable-xmlwriter --with-openssl --with-readline --enable-mbstring --without-pear --with-curl --enable-mbstring --host=x86_64-linux-gnu --target=x86_64-linux-gnu
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

#### mysql-connector-c
``` Bash
mkdir -p /data/vendor/mysqlc-8.0.15/lib
cp -R mysql-8.0.15-linux-glibc2.12-x86_64/include /data/vendor/mysqlc-8.0.15
cp mysql-8.0.15-linux-glibc2.12-x86_64/lib/libmysqlclient.a /data/vendor/mysqlc-8.0.15/lib
```

#### mongoc-driver
``` Bash
mkdir stage && cd stage
CC=gcc CXX=g++ cmake -DCMAKE_INSTALL_PREFIX=/data/vendor/mongoc-1.14.0 -DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS=-fPIC -DENABLE_STATIC=ON -DENABLE_SHM_COUNTERS=OFF -DENABLE_TESTS=OFF -DENABLE_EXAMPLES=OFF -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF ../
# 由于 mysql-connector-c 官方提供的版本需要 openssl-1.0 此处也须统一版本
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
cp src/snappy.h /data/vendor/rdkafka-1.0.0/include/librdkafka/
cp src/rdmurmur2.h /data/vendor/rdkafka-1.0.0/include/librdkafka/
cp src/xxhash.h /data/vendor/rdkafka-1.0.0/include/librdkafka/
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
