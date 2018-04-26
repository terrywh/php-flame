<?php
flame\init("http-server");
flame\go(function() {
	// 创建 http 处理器
	$handler = new flame\net\http\handler();
	// 设置处理程序
	$handler->handle(function($req, $res) { // 默认 Handler
		yield $res->write_header(404);
	})->get("/hello", function($req, $res) {
		yield $res->write("hello world");
	})->before(function($req, $res, $match) { // 每个请求处理前被调用
		// 用 $req->data 存储流程数据
		$req->data["begin"] = flame\time\now();
		yield $res->end("2");
	})->after(function($req, $res, $match) { // 请求处理结束后（销毁前）调用
		$elapse = flame\time\now() - $req->data["begin"];
		yield $res->end(strval($elapse)."ms");
	});
	// 创建网络服务器
	$server = new flame\net\tcp_server();
	$server->handle($handler); // 指定服务程序
	$server->bind("::", 19002);
	yield $server->run();
});
flame\run();
