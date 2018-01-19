<?php
flame\init("http-server");
flame\go(function() {
	// 创建 http 处理器
	$handler = new flame\net\http\handler();
	// 设置处理程序
	$handler->handle(function($req, $res) { // 默认 Handler
		yield $res->write(mt_rand());
	})->before(function($req, $res) { // 每个请求处理前被调用
		// 数组 $req->data 可用于携带和传递数据给 handler / after
		$req->data["begin"] = flame\time\now();
		yield $res->write("[[[[[[");
	})->after(function($req, $res) { // 请求处理结束后（销毁前）调用
		yield $res->end("]]]]]]");
		$elapse = flame\time\now() - $req->data["begin"];
		echo "request for '{$req->uri}' complete in {$elapse}ms\n";
	});
	// 创建网络服务器
	$server = new flame\net\tcp_server();
	$server->handle($handler); // 指定服务程序
	$server->bind("::", 19002);
	yield $server->run();
});
flame\run();
