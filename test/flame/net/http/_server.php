<?php
flame\go(function() {
	$server = new flame\net\http\server();
	// 每个请求会回调到单独的协程函数中进行处理
	$server->handle(function($request, $response) {
		var_dump($request, $response);
	});
	$server->handle("/", function($request, $response) {
		var_dump($request, $response);
	});
	$server->bind("::", 7678);
	// 在 $server->close() 之前，会“阻塞”在此处
	yield $server->run();
});
flame\run();