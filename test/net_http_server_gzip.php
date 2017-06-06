<?php
dl("flame.so");

flame\run(function() {
	$server = new flame\http\server();
	echo "server created\n";
	$server->handle("/test1", function($request, $response) {
		// $request->header 对象实现了 Iterator 及 ArrayAccess 接口
        $response->enable_gzip();
        $data = file_get_contents(__DIR__."/http_server.php");
        yield $response->write($data);
        $data = file_get_contents(__DIR__."/core.php");
        yield $response->write($data);
        $data = file_get_contents(__DIR__."/db_lmdb.php");
        yield $response->write($data);
        yield $response->write("0987654321");
        yield $response->write("zyxwvutsrqponmlkjihgfedcba");
		yield $response->end("1");
		//var_dump($request);
        //var_dump($response);
	});
	echo "handler added\n";
	yield $server->listen_and_serve("0.0.0.0", 16666);
	echo "closed\n";
});
