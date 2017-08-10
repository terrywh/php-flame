<?php
flame\go(function() {
	$server = new flame\net\fastcgi\server();

	$server->handle("/hello", function($req, $res) {
		$res->write("hello ");
		$res->end("world\n");
	});
	$server->handle(function($req, $res) {
		$data = json_encode($req);
		yield $res->end($data);
	});
	@unlink("/data/sockets/flame.xingyan.panda.tv.sock");
	$server->bind("/data/sockets/flame.xingyan.panda.tv.sock");
	@chmod("/data/sockets/flame.xingyan.panda.tv.sock", 0777);
	// 可选 fork 多进程
	yield $server->run();
});
flame\run();
