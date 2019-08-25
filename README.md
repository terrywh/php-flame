### FLAME
为解决 PHP 被认为“并发处理能力不足”的问题，我最初在业务中试用 [Swoole](https://www.swoole.com/) 框架；估计由于使用的是 Swoole 相对早期的版本，在实际业务中出现了各式各样的问题（很多都是内存问题）；同时很多同学对 Swoole 中提供的进程使用形式，都感觉把握比较吃力；  
于是在各方支持下，开发了 FLAME 框架：

* 基于协程机制的异步化网络底层（ TCP / UDP / HTTP)；
* 简单的多进程模式 （ SO_REUSEPORT ）；
* 基于协程的队列机制（ 类似 Go 的 Channel）；
* 多种驱动异步化支持（ MySQL / Redis / MongoDB / RabbitMQ / Kafka )；

同时为了简化日常应用开发成本，及很多重复性工作，框架还提供了：
* 平滑起停；
* 多进程日志、登等级过滤、文件重载；
* 数据库连接池机制；
* 异步进程；
* 等等（BSON / SNAPPY / INTERFACE）...

目前，FLAME 已经使用在了公司内各种业务线；实际使用中，框架在**并行处理性能**方面表现十分**突出**，并在持续的改进、完善下，框架在**稳定性**方面也有了**不错**的表现；  
不过，由于时间、能力问题，未能详尽的测试整个框架的各项功能，可能还存在一些问题、缺陷；感谢大家的包容、反馈与帮助（大礼参拜）

### 示例
以下代码实现了几个简单的 HTTP 接口，展示了框架的基本使用方法：

``` PHP
<?php
// 框架初始化
flame\init("http_server_demo");
// 第一个（主）协程
flame\go(function() {
    // 创建 HTTP 服务器（监听）
    $server = new flame\http\server(":::56101");

    $server
        ->before(function($req, $res) { // 前置处理器（HOOK）
            $req->data["before"] = flame\time\now(); // 记录请求开始时间
        })
        ->get("/hello", function($req, $res) { // 路径处理器
            // 简单响应方式
            $res->status = 200;
            $res->body = "world";
        })
        ->post("/hello/world", function($req, $res) { // 路径处理器
            // Transfer-Encoding: Chunked
            $res->write_header(200);
            $res->write("CHUNKED RESPONSE:")
            $res->write($res->body);
            $res->end();
        })
        ->after(function($req, $res, $r) {
            // 后置处理器（HOOK）
            flame\log\trace($req->method, $req->path // 请求时长日志记录
                , "in", (flame\time\now() - $req->data["before"]), "ms");
            if(!$r) {
                $res->status = 404;
                $res->file(__DIR__."/404.html"); // 响应文件
            }
        });
    $server->run();
});
// 启动（调度）
flame\run();
```

### 功能

* **core** - 核心，框架初始化设置，协程启动，协程队列，协程锁等；
* **time** - 时间相关，协程休眠，当前时间等；
> 毫秒级时间戳及缓存机制;  
* **log** - 简单日志记录功能:
> 多进程统一日志记录;  
> 支持日志登记过滤设置;  
> 支持通过信号进行日志重载;  
* **os** - 操作系统相关信息获取，异步进程启停操作等；
> 获取本地网卡地址;
* **tcp** - 封装 TCP 相关服务端客户端功能；
* **udp** - 封装 UDP 相关服务端客户端功能；
* **http** - 简单的 HTTP 客户端/服务端封装;
> 客户端支持长连 Keep-Alive 及相关连接数控制;  
> 客户端支持 HTTPS 及 HTTP/2 协议部分功能;  
> 客户端暂不支持 form-data/multipart 形式的请求;  
> 服务器**不支持** HTTPS 可考虑使用 NGINX 等进行反向代理;  
* **mysql** - 简单 MySQL 客户端:
> 提供部分简化方法，如 `insert/delete/one` 等, 自动进行 ESCAPE 转义;  
> 内部使用连接池形式自动连接复用;  
> 支持事务;  
* **redis** - 简单 Redis 客户端:
> 内部使用连接池形式自动连接复用;  
> 暂不支持 SUBSCRIBE 相关指令;  
* **mongodb** - 简单 MongoDB 客户端:
> 提供部分简化方法，如 `insert/delete/one/count` 等;  
> 内部使用连接池自动连接复用;  
* **rabbitmq** - 简单 RabbitMQ 生产消费支持:
> 支持使用协程进行"并行"消费;
* **kafka** - 简单 Kafka 生产消费支持:
> 仅支持 Kafka 0.10+ 群组消费形式;  
> 支持使用协程进行"并行"消费;  
* **hash** - 提供了若干哈希算法；
* **encoding** - 提供了若干编码、序列化函数；
* **compress** - 提供了若干压缩算法；

### 说明
* [API 文档](https://github.com/terrywh/php-flame/tree/master/doc)
* [常见问题](https://github.com/terrywh/php-flame/wiki/%E5%B8%B8%E8%A7%81%E9%97%AE%E9%A2%98)
* 自动完成（IDE提示）- 目录 `/doc` 下使用 PHP Doc 语法定义和说明 API 接口，可挂接 IDE 自动完成功能；
    * VSCode/ (php-intelephense)
    > ``` Bash
    > ln -s /path/to/php-flame/doc /path/to/extensions/bmewburn.vscode-intelephense-client-x.x.xx/node_modules/intelephense/lib/stub/flame
    > ```
    * PhpStorm
    > [Configuring Include Paths](https://www.jetbrains.com/help/phpstorm/configuring-include-paths.html#Configuring_Include_Paths.xml)

### 其他
<details><summary>依赖项编译安装，仅供参考</summary>
<p>

#### boost
``` Bash
./bootstrap.sh --prefix=/data/vendor/boost-1.70.0
./b2 --prefix=/data/vendor/boost-1.70.0 cxxflags="-fPIC" variant=release link=static threading=multi install
```

#### cpp-parser
``` Bash
make install
```

#### lltoml
``` Bash
CFLAGS="-O2 -DNDEBUG" CXXFLAGS="-O2 -DNDEBUG" make
make install
```

#### hiredis
``` Bash
CC=gcc make
PREFIX=/data/vendor/hiredis-0.14.0 make install
# 未提供禁用动态库选项
# rm /data/vendor/hiredis-0.14.0/lib/*.so*
```

#### openssl
``` Bash
CC=gcc CXX=g++ ./Configure no-shared --prefix=/data/vendor/openssl-1.1.1c linux-x86_64
make && make install
```

#### AMQP-CPP
``` Bash
mkdir stage && cd stage
CC=gcc CXX=g++ CXXFLAGS="-fPIC -I/data/vendor/openssl-1.1.1c/include" LDFLAGS="-L/data/vendor/openssl-1.1.c/lib" cmake -DCMAKE_INSTALL_PREFIX=/data/vendor/amqpcpp-4.1.5 -DCMAKE_BUILD_TYPE=Release -DAMQP-CPP_LINUX_TCP=ON ../
make && make install
```

<!--
#### sasl2
``` Bash
PKG_CONFIG_PATH=/data/vendor/openssl-1.1.1c/lib/pkgconfig CC=gcc CXX=g++ CFLAGS=-fPIC CXXFLAGS=-fPIC ./configure --prefix=/data/vendor/sasl2 --with-openssl=/data/vendor/openssl-1.1.1c --without-ldap --enable-shared=no
make && make install
```
-->

#### mongoc-driver
``` Bash
mkdir stage && cd stage
CC=gcc CXX=g++ CFLAGS="-fPIC" LDFLAGS="-pthread -ldl" PKG_CONFIG_PATH=/data/vendor/openssl-1.1.1c/lib/pkgconfig cmake -DCMAKE_INSTALL_PREFIX=/data/vendor/mongoc-1.15.0 -DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_BUILD_TYPE=Release -DENABLE_STATIC=ON -DENABLE_SASL=OFF -DENABLE_SHM_COUNTERS=OFF -DENABLE_TESTS=OFF -DENABLE_EXAMPLES=OFF -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF ../
make && make install
# 未提供 ENABLE_SHARED=OFF 或类似选项
# rm /data/vendor/mongoc-1.15.0/lib/*.so*
```

#### rdkafka
``` Bash
CC=gcc CXX=g++ PKG_CONFIG_PATH=/data/vendor/openssl-1.1.1c/lib/pkgconfig ./configure --prefix=/data/vendor/rdkafka-1.1.0 --disable-sasl
make && make install
# rm /data/vendor/rdkafka-1.1.0/lib/*.so*
cp src/snappy.h /data/vendor/rdkafka-1.1.0/include/librdkafka/
cp src/rdmurmur2.h /data/vendor/rdkafka-1.1.0/include/librdkafka/
cp src/xxhash.h /data/vendor/rdkafka-1.1.0/include/librdkafka/
```

#### PHP
``` Bash
CC=gcc CXX=g++ CFLAGS="-pthread" ./configure --prefix=/data/server/php-7.2.21 --with-config-file-path=/data/server/php-7.2.21/etc --disable-simplexml --disable-xml --disable-xmlreader --disable-xmlwriter --with-readline --enable-mbstring --without-pear --with-zlib --with-openssl=/data/vendor/openssl-1.1.1c --build=x86_64-linux-gnu
make && make install
```

<!--
# external openssl extension for php
CC=gcc CXX=g++ LDFLAGS="-pthread -ldl" ./configure --with-php-config=/data/server/php-7.2.19/bin/php-config --with-openssl=/data/vendor/openssl-1.1.1c --build=x86_64-linux-gnu
-->

#### libphpext
``` Bash
CXXFLAGS="-O2 -DNDEBUG" make
make install
```

#### c-ares
``` Bash
mkdir stage && cd stage
CC=gcc CXX=g++ cmake -DCARES_SHARED=OFF -DCARES_STATIC=ON -DCARES_STATIC_PIC=ON -DCMAKE_INSTALL_PREFIX=/data/vendor/cares-1.15.0 -DCMAKE_BUILD_TYPE=Release ../
make && make install
```

#### nghttp2
``` Bash
mkdir stage && cd stage
CC=gcc CXX=g++ CFLAGS="-fPIC" CXXFLAGS="-fPIC" PKG_CONFIG_PATH="/data/vendor/cares-1.15.0/lib/pkgconfig:/data/vendor/openssl-1.1.1c/lib/pkgconfig" cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/data/vendor/nghttp2-1.39.2 -DENABLE_LIB_ONLY=ON -DENABLE_SHARED_LIB=OFF -DENABLE_STATIC_LIB=ON -DCMAKE_INSTALL_LIBDIR:PATH=lib ../
make && make install
```

#### curl
``` Bash
# quote: cmake for curl is poorly maintained
CC=gcc CXX=g++ CFLAGS=-fPIC CPPFLAGS=-fPIC ./configure --with-ssl=/data/vendor/openssl-1.1.1c --enable-ares=/data/vendor/cares-1.15.0 --with-nghttp2=/data/vendor/nghttp2-1.39.2 --enable-shared=no --enable-static --enable-ipv6 --without-brotli --without-libidn2 --without-libidn --without-librtmp --disable-unix-sockets --disable-ftp --disable-ldap --disable-ldaps --disable-rtsp --disable-dict --disable-file --disable-telnet --disable-tftp --disable-pop3 --disable-imap --disable-smb --disable-gopher --without-libpsl --prefix=/data/vendor/curl-7.65.3
make && make install
```

#### maria-connector-c
``` Bash
mkdir stage && cd stage
CC=gcc CXX=g++ CFLAGS="-pthread" CXXFLAGS="-pthread" PKG_CONFIG_PATH=/data/vendor/openssl-1.1.1c/lib/pkgconfig:/data/vendor/curl-7.65.3/lib/pkgconfig cmake -DCMAKE_BUILD_TYPE=Release -DCLIENT_PLUGIN_SHA256_PASSWORD=STATIC -DCLIENT_PLUGIN_CACHING_SHA2_PASSWORD=STATIC -DCMAKE_INSTALL_PREFIX=/data/vendor/mariac-3.1.3 ../
make && make install
# rm /data/vendor/mariac-3.1.3/lib/mariadb/*.so*
```

</p>
</details>

### 版权声明
本软件使用 MIT 许可协议；
