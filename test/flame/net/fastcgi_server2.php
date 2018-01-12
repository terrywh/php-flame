<?php
flame\init("fastcgi-server");
flame\go(function() {
	// 创建 fastcgi 处理器
	$handler = new flame\net\fastcgi\handler();
	// 设置处理程序
	$handler->handle(function($req, $res) {// 默认处理程序（即：不匹配上述路径形式时调用）
		yield $res->write("1");
		yield $res->write("2");
		yield $res->write("3");
		yield $res->end();
	});
	// 创建网络服务器
	$server = new flame\net\tcp_server();
	$server->handle($handler); // 指定服务程序
	$server->bind("127.0.0.1", 19001);
	yield $server->run();
});
flame\run();
