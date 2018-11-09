## Flame
**Flame** 是一个 PHP 框架，借用 PHP Generator 实现**协程式**的程序开发。

## 文档
https://terrywh.github.io/php-flame/


## 示例
``` PHP
<?php
// 框架初始化（自动设置进程名称）
flame\init("http-server", [
	"debug" => false, // 非调试状态（自动重启）
	"worker" => 4, // 多进程服务
]);
// 启用一个协程作为入口
flame\go(function() {
	// 创建 http 服务器
	$server = new flame\http\server(":::7678");
	// 设置默认处理程序
	$server->before(function($req, $res, $match) {
		if($match) {
			$req->data["time"] = flame\time\now();
		}
	})->get("/hello", function($req, $res) {
		yield $res->write("hello ");
		yield $res->end(" world");
	})->after(function($req, $res, $match) {
		if($match) {
			flame\log\info("cost", flame\time\now() - $req->data["time"], "ms");
		}else{
			$res->status = 404;
			$res->body = "not found";
		}
	});
	// 启动并运行服务器
	yield $server->run();
});
// 框架调度执行
flame\run();
```

* HTTP 服务器：[examples/http_server.php](https://github.com/terrywh/php-flame/blob/master/examples/http_server.php)

## 依赖

* [php](https://www.php.net)

* [boost](https://www.boost.org/)
```
sudo -s
source /home/wuhao/.bashrc
./bootstrap.sh --with-toolset=clang --prefix=/data/vendor/boost-1.68.0
./b2 --prefix=/data/vendor/boost-1.68.0 cxxflags=-fPIC toolset=clang variant=release link=static threading=multi install
```

* [libphpext](https://github.com/terrywh/libphpext.git) - 依赖于 PHP
```
CXXFLAGS="-O2" make -j4
make PREFIX=/data/vendor/libphpext-x.x.x install
```

* [cpp-parser](https://github.com/terrywh/cpp-parser.git) - 纯头文件库
```
make PREFIX=/data/vendor/parser-x.x.x install
```

* [AMQP-CPP](https://github.com/CopernicaMarketingSoftware/AMQP-CPP.git) - 仅编译静态库
```
mkdir build && cd build
CC=gcc CXX=g++ cmake -DCMAKE_INSTALL_PREFIX=/data/vendor/amqpcpp-3.2.0 -DCMAKE_CXX_FLAGS=-fPIC -DCMAKE_BUILD_TYPE=Release ../
make
make install
// 仅保留静态库
rm /data/vendor/librdkafka-0.11.6/lib/librdkafka*.so*
```

* [MySQL-Connector-C](https://downloads.mysql.com/archives/c-c/) - 6.1.11 Linux - Generic 64bit (注意仅保留静态库)
```
```
*目前 8.0 版本未单独提供 libmysqlclient 通用库 (包含在服务器中的版本链接的 ssl 库版本与 CentOS7 不直接兼容);*


* [mongo-c-driver](http://mongoc.org/libmongoc/current/index.html) - 仅保留静态库
```
mkdir stage && cd stage
cd mongo-c-driver-1.11.0-build
CC=gcc CXX=g++ cmake -DCMAKE_INSTALL_PREFIX=/data/vendor/mongoc-1.13.0 -DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS=-fPIC -DENABLE_STATIC=ON -DENABLE_SHM_COUNTERS=OFF -DENABLE_TESTS=OFF -DENABLE_EXAMPLES=OFF -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF ../
make
make install
rm -f /data/vendor/mongoc-1.13.0/lib/libbson-1.0.so*
rm -f /data/vendor/mongoc-1.13.0/lib/libmongoc-1.0.so*
```

#### 编译
```
cd /path/to/php-flame
git pull
make clean
rm -f flame-debugd.so flame-formal.so
make -j4
make install
mv flame.so flame-debugd.so
make clean
CXXFLAGS="-O2" make -j4
mv flame.so flame-formal.so
```

#### 推送
```
scp flame-debugd.so 11.22.33.44:~/
ssh -t 11.22.33.44 "sudo rm /path/to/php/lib/php/extensions/no-debug-non-zts-20151012/flame.so; sudo cp ~/flame-debugd.so /path/to/php/lib/php/extensions/no-debug-non-zts-20151012/flame.so"
```
