<?php
flame\go(function() {
	$server = new flame\net\fastcgi\server();
	$server->handle("/hello", function($req, $res) {
		$res->write("1000");
	});
	$server->handle(function($req, $res) {
		
	});
	@unlink("/data/sockets/flame.xingyan.panda.tv.sock");
	$server->bind("/data/sockets/flame.xingyan.panda.tv.sock");
	@chmod("/data/sockets/flame.xingyan.panda.tv.sock", 0777);
	yield $server->run();
});
flame\run();