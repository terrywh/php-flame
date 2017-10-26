<?php
flame\init("fastcgi-server");
flame\go(function() {
	// 创建 fastcgi 处理器
	$handler = new flame\net\fastcgi\handler();
	// 设置处理程序
	$a = $handler->get("/hello", function($req, $res) {
		yield flame\time\sleep(2000);
		var_dump($req->method, $req->body); // GET ....
		yield $res->write("hello ");
		yield $res->end("world\n");
	})
	->post("/hello", function($req, $res) {
		yield flame\time\sleep(2000);
		var_dump($req->method, $req->body); // POST ....
		yield $res->write("hello ");
		yield $res->end("world\n");
	})
	->get("/favicon.ico", function($req, $res) {
		yield $res->write_header(404);
		yield $res->end();
	})
	// 默认处理程序（即：不匹配上述路径形式时调用）
	->handle(function($req, $res) {
		yield flame\time\sleep(2000);
		var_dump($req);
		$data = json_encode($req);
		yield $res->end($data);
	});
	// 创建网络服务器
	$server = new flame\net\tcp_server();
	$server->handle($handler); // 指定服务程序
	$server->bind("0.0.0.0", 19001);
	yield $server->run();
});
flame\run();
