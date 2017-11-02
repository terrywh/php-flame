## Flame
**Flame** 是一个 PHP 框架，借用 PHP Generator 实现 协程式 的编程服务。目前，flame 中提供了如下功能：
1. 协程核心；
	1. 核心协程函数；
	2. [时间协程函数](/php-flame/flame_time)（如 定时器等）；
2. [协程式网络](/php-flame/flame_net)；
	1. [HTTP 客户端](/php-flame/flame_net_http)；
	2. Unix Socket 客户端、服务端；
	3. TCP 客户端、服务端；
	4. UDP 客户端、服务端；
	5. [FastCGI 处理器](/php-flame/flame_net_fastcgi) - 挂接 Nginx 等实现 HTTP 服务；
3. [协程式数据库驱动](/php/flame/flame_db)：
	1. 简单 Redis 客户端；
	2. 简单 Mongodb 客户端；
	3. 简单 MySQL 客户端；

**源码**
https://github.com/terrywh/php-flame/

**示例**：
https://github.com/terrywh/php-flame/tree/master/test/flame

**注意**：
* 文档中带 `yield` 前缀的函数为“异步”、“协程式”函数，请在调用时也保持 `yield` 关键字；
* 嵌套异步 `Generator` 可使用 `yield` 关键字直接调用，也可以使用 `PHP` 的嵌套语法 `yield from`；具体可参考 PHP 文档： [Gernerator Syntax](http://php.net/manual/en/language.generators.syntax.php) 的相关说明；

**示例**：
``` PHP
<?php
flame\init("fastcgi-server");
flame\go(function() {
	// 创建 fastcgi 处理器
	$handler = new flame\net\fastcgi\handler();
	// 设置默认处理程序
	$handler->handle(function($req, $res) {
		yield flame\time\sleep(2000);
		var_dump($req);
		yield $res->end("hello world!");
	});
	// 创建网络服务器
	$server = new flame\net\tcp_server();
	$server->handle($handler); // 指定处理程序
	$server->bind("127.0.0.1", 19001);
	yield $server->run();
});
flame\run();
```
完整示例可参考 [/test/flame/net/fastcgi](https://github.com/terrywh/php-flame/tree/master/test/flame/net/fastcgi)` 相关代码；

## `namespace flame`

最基本的“协程”函数封装，例如生成“协程”，“协程”调度等；

#### `flame\init(string $app_name, array $options)`
可选，设置并初始化框架；

**实例**：
``` php
<?php
flame\init("test_app", [
	"worker" => 4, // 可选，工作进程数，默认为 0
]);

#### `flame\go(callable $g)`
生成一个“协程”；

**示例**：
``` php
<?php
function g1() {
	yield 1;
	yield 2;
}
// 传入 Generator 函数的定义：
flame\go(g1);
flame\go(function() {
	yield 3;
	yield 4;

});
```
**注意**：
* 本函数必须在下述 `flame\run()` 函数前执行；否则无法生效；

#### `flame\on_quit(callable $cb)`
添加退出前的回调函数；一般用于在需要终止进程时，进行补充操作、记录日志等，例如 `$server->close()` 等；

**注意**：
* 本函数可重复调用，添加多个回调函数；
* 函数按添加顺序相反的顺序执行（实际为 `stack` 容器）；
* 当进程要结束时还有异步流程没有执行完毕，将会在 10s 后强制终止；

#### `flame\run()`
框架入口，所有协程调度在此处开始执行，直到所有“协程”结束；

**示例**：
``` PHP
<?php
flame\go(function() {
	// yield ...
});
flame\run(); // 实际程序在此循环调度
```

**注意**:
* 此函数调用一般放置在入口执行文件的最后，开始后本函数会阻塞，直到所有协程执行结束；
