<?php
flame\init("fastcgi-server", [
	"worker" => 2, // 启动两个工作进程
]);
flame\go(function() {
	$server = new flame\net\fastcgi\server();

	$server->handle("/hello", function($req, $res) {
		var_dump($req->method, $req->body); // GET POST PUT ....
		yield $res->write("hello ");
		yield $res->end("world\n");
	})
	->handle("/favicon.ico", function($req, $res) {
		yield $res->write_header(404);
		yield $res->end();
	})
	->handle(function($req, $res) {
		echo "----- ", getmypid(), " -----\n";
		$data = json_encode($req);
		yield $res->end($data);
	});
	// 方式1. 绑定 unix socket
	$server->bind("/data/sockets/flame.xingyan.panda.tv.sock");
	@chmod("/data/sockets/flame.xingyan.panda.tv.sock", 0777);
	// 方式2. 绑定 tcp
	// $server->bind("127.0.0.1", 19001);
	yield $server->run();
});
flame\run();
