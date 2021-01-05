### FLAME

* 隐式协程调度机制（类似 `Go` 协程控制）；
* 常见网络服务 (`TCP` / `UDP` / `HTTP` / `SMTP`)；
* 简单平行工作进程，支持平滑起停，多进程日志；
* 常见驱动支持 (`MySQL` / `Redis` / `MongoDB` / `RabbitMQ` / `Kafka`)；
* 底层完全异步化，高并发支持；原生交互优化，低性能损耗；

框架提供了相对简单的 API 设计，同时为了简化日常、重复性工作：
* 平滑起停，自定义进程名称；
* 日志等级过滤、文件重载
* 进程间 IPC 消息通讯；
* 数据库连接池机制；
* 异步进程；
* 标准时间、毫秒时间戳、定时器；
* SMTP 邮件发送;
* TOML 配置解析；
* BSON 编解码；
* SNAPPY 简单压缩解压；
* 额外的哈希 CRC64 / MURMUR2 / XXH64 函数；

等等；

### 背景
`PHP` 经常被认为“并发处理能力不足”，一般理由：

* 基于 `FastCGI` 进程服务，不支持 `multiplex` 机制；
* 无原生并发处理机制（无线程、协程、异步支持）；

在业务中尝试过各种类型的解决方案：
* Workerman
* Yar
* libevent/eio (php-extension)
* HHVM
* hi-nginx
* Swoole

由于了解较少、出现BUG、不方便、文档缺乏、部署麻烦、驱动缺乏、兼容问题等等各式各样的原因逐渐放弃；  
也进行了不少 `Go`/`Node.js` 业务迁移、实现，不乏非常成功案例；  
长时间的开发过程中，直观的感觉到 `PHP` 在 “**简单方便**” 这一点上还是十分出色的；  

于是，在各方鼓励、支持、帮助下，自主研发了 `FLAME` 框架；

目前，框架 `FLAME` 已经应用在了公司内各类业务线 (前后端接口、内部生产消费、后台计算、管理工具等)；  
在实际使用中，框架 `FLAME` 在**并行处理性能**方面表现十分**出色**，**稳定性**方面也有**不错**的表现；  
由于时间、能力问题，框架可能还存在一些问题、缺陷；感谢大家的包容、反馈与帮助（大礼参拜）~

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

### 文档
* 组成部分
![组成部分](https://github.com/terrywh/php-flame/raw/v0.18.x/README-structures.png)
* [API 文档](https://github.com/terrywh/php-flame/tree/master/doc)
* [常见问题](https://github.com/terrywh/php-flame/wiki/%E5%B8%B8%E8%A7%81%E9%97%AE%E9%A2%98)
* 自动完成（IDE提示）- 目录 `/doc` 下使用 PHP Doc 语法定义和说明 API 接口，可挂接 IDE 自动完成功能；
    * VSCode/ (php-intelephense)
    > ``` Bash
    > ln -s /path/to/php-flame/doc /path/to/extensions/bmewburn.vscode-intelephense-client-x.x.xx/node_modules/intelephense/lib/stub/flame
    > ```
    * PhpStorm
    > [Configuring Include Paths](https://www.jetbrains.com/help/phpstorm/configuring-include-paths.html#Configuring_Include_Paths.xml)

* 依赖
> * PHP >= v8.0 (注意 `--with-pic` 配置参数)

### 其他
<details><summary>依赖项编译安装，仅供参考</summary>
<p>

#### openssl
``` Bash
CC=gcc CXX=g++ ./Configure no-shared --prefix=/data/vendor/openssl-1.1 linux-x86_64
make && make install
```

#### boost
``` Bash
./bootstrap.sh --prefix=/data/vendor/boost-1.74
./b2 --prefix=/data/vendor/boost-1.74 cxxflags="-fPIC" variant=release link=static threading=multi install
```

#### fmt
``` Bash
mkdir stage && cd stage
CC=gcc CXX=g++ CFLAGS="-fPIC" CXXFLAGS="-fPIC" cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/data/vendor/fmt-7.1 ../
make && make install
```

#### c-ares
``` Bash
mkdir stage && cd stage
CC=gcc CXX=g++ cmake \
    -DCARES_SHARED=OFF -DCARES_STATIC=ON -DCARES_STATIC_PIC=ON \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/data/vendor/cares-1.17 \
    ../
make && make install
```

#### nghttp2
``` Bash
mkdir stage && cd stage
CC=gcc CXX=g++ CFLAGS=-fPIC CXXFLAGS=-fPIC PKG_CONFIG_PATH="/data/vendor/cares-1.17/lib/pkgconfig:/data/vendor/openssl-1.1/lib/pkgconfig" ./configure \
    --prefix=/data/vendor/nghttp2-1.42 \
    --disable-shared \
    --enable-static \
    --enable-lib-only \
    --with-boost=/data/vendor/boost-1.74 \
    --enable-asio-lib
make && make install
```

#### curl
``` Bash
# quote: cmake for curl is poorly maintained
CC=gcc CXX=g++ CFLAGS=-fPIC CPPFLAGS=-fPIC ./configure \
    --with-ssl=/data/vendor/openssl-1.1 \
    --enable-ares=/data/vendor/cares-1.17 \
    --with-nghttp2=/data/vendor/nghttp2-1.42 \
    --disable-shared --enable-static --enable-ipv6 \
    --without-brotli --without-libidn2 --without-libidn --without-librtmp --without-libpsl \
    --disable-unix-sockets --disable-ftp --disable-ldap --disable-ldaps \
    --disable-rtsp --disable-dict --disable-file --disable-telnet --disable-tftp \
    --disable-pop3 --disable-imap --disable-smb --disable-gopher \
    --prefix=/data/vendor/curl-7.73
make && make install
```

#### lmdb

``` Bash
# wget https://github.com/LMDB/lmdb/archive/mdb.master.zip
cd lmdb-mdb.master/libraries/liblmdb
XCFLAGS="-fPIC" make -j4
make install prefix=/data/vendor/lmdb-0.9
```

#### PHP
``` Bash
CC=gcc CXX=g++ PKG_CONFIG_PATH=/data/vendor/openssl-1.1/lib/pkgconfig './configure' '--prefix=/data/server/php-8.0' '--with-config-file-path=/data/server/php-8.0/etc' '--build=x86_64-linux-gnu' '--with-pic' '--disable-all' '--disable-simplexml' '--disable-xml' '--disable-xmlreader' '--disable-xmlwriter' '--without-libxml' '--without-pear' '--enable-cli' '--enable-opcache' '--enable-opcache-jit' '--enable-mbstring' '--with-readline' '--with-zlib' '--with-openssl=/data/vendor/openssl-1.1' 
# 不使用 --with-pic 参数可能出现部分奇怪的符号链接问题；
make && make install
```

#### libphpext
``` Bash
mkdir stage && cd stage
cmake -DCMAKE_BUILD_TYPE=Release ../
make && make install
```

#### cpp-parser
``` Bash
make install
```

#### lltoml
``` Bash
ENVIRON="canvas" CFLAGS="-O2 -DNDEBUG" CXXFLAGS="-O2 -DNDEBUG" make
ENVIRON="canvas" make install
```

#### hiredis
``` Bash
CC=gcc make
PREFIX=/data/vendor/hiredis-0.14 make install
# 未提供禁用动态库选项
# rm /data/vendor/hiredis-0.14/lib/*.so*
```


#### AMQP-CPP
``` Bash
mkdir stage && cd stage
CC=gcc CXX=g++ CXXFLAGS="-fPIC -I/data/vendor/openssl-1.1/include" LDFLAGS="-L/data/vendor/openssl-1.1/lib" cmake -DCMAKE_INSTALL_PREFIX=/data/vendor/amqpcpp-4.1 -DCMAKE_BUILD_TYPE=Release -DAMQP-CPP_LINUX_TCP=ON ../
make && make install
```

<!--
#### sasl2
``` Bash
PKG_CONFIG_PATH=/data/vendor/openssl-1.1/lib/pkgconfig CC=gcc CXX=g++ CFLAGS=-fPIC CXXFLAGS=-fPIC ./configure --prefix=/data/vendor/sasl2 --with-openssl=/data/vendor/openssl-1.1 --without-ldap --enable-shared=no
make && make install
```
-->

#### mongoc-driver
``` Bash
mkdir stage && cd stage
CC=gcc CXX=g++ CFLAGS="-fPIC" LDFLAGS="-pthread -ldl" PKG_CONFIG_PATH=/data/vendor/openssl-1.1/lib/pkgconfig cmake -DCMAKE_INSTALL_PREFIX=/data/vendor/mongoc-1.17 -DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_BUILD_TYPE=Release -DENABLE_STATIC=ON -DENABLE_SASL=OFF -DENABLE_SHM_COUNTERS=OFF -DENABLE_TESTS=OFF -DENABLE_EXAMPLES=OFF -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF ../
make && make install
# 未提供 ENABLE_SHARED=OFF 或类似选项
# rm /data/vendor/mongoc-1.17/lib/*.so*
```

#### rdkafka
``` Bash
CC=gcc CXX=g++ PKG_CONFIG_PATH=/data/vendor/openssl-1.1/lib/pkgconfig ./configure --prefix=/data/vendor/rdkafka-1.5 --disable-sasl
make && make install
# rm /data/vendor/rdkafka-1.5/lib/*.so*
cp src/snappy.h /data/vendor/rdkafka-1.5/include/librdkafka/
cp src/rdmurmur2.h /data/vendor/rdkafka-1.5/include/librdkafka/
cp src/rdxxhash.h /data/vendor/rdkafka-1.5/include/librdkafka/
```

#### maria-connector-c
``` Bash
mkdir stage && cd stage
CC=gcc CXX=g++ CFLAGS="-pthread" CXXFLAGS="-pthread" PKG_CONFIG_PATH=/data/vendor/openssl-1.1/lib/pkgconfig:/data/vendor/curl-7.71/lib/pkgconfig cmake -DCMAKE_BUILD_TYPE=Release -DCLIENT_PLUGIN_SHA256_PASSWORD=STATIC -DCLIENT_PLUGIN_CACHING_SHA2_PASSWORD=STATIC -DCMAKE_INSTALL_PREFIX=/data/vendor/mariac-3.1 ../
make && make install
# rm /data/vendor/mariac-3.1/lib/mariadb/*.so*
```

</p>
</details>

### 版权声明
本软件使用 MIT 许可协议；
