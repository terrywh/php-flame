<?php
flame\init("http-server");
flame\go(function() {
	// 创建 http 处理器
	$handler = new flame\net\http\handler();
	// 设置处理程序
	$handler->handle(function($req, $res) {
		yield $res->end("1");
	});
	// 创建网络服务器
	$server = new flame\net\tcp_server();
	$server->handle($handler); // 指定服务程序
	$server->bind("::", 19002);
	yield $server->run();
});
flame\run();
