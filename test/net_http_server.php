<?php
dl("flame.so");
// flame\fork(1);
flame\run(function() {
	$server = new flame\http\server();
	echo "server created\n";
	$server->handle("/test1", function($request, $response) {
		yield $response->end("hello world!");
	});
	$server->handle("/test2", function($request, $response) {
		// 忘记 $response
	});
	echo "handler added\n";
	yield $server->listen_and_serve("::", 6676);
	echo "closed\n";
});
