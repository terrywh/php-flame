## Flame
**Flame** 是一个 PHP 框架，借用 PHP Generator 实现 协程式 的编程服务。

## 文档
https://terrywh.github.io/php-flame/


## 示例
``` PHP
<?php
flame\init("fastcgi-server");
flame\go(function() {
	// 创建 http 处理器
	$handler = new flame\net\http\handler();
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
