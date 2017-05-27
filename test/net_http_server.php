<?php
dl("flame.so");
// flame\fork(1);
flame\run(function() {
	$server = new flame\http\server();
	echo "server created\n";
	$server->handle("/test1", function($request, $response) {
		// $request->header 对象实现了 Iterator 及 ArrayAccess 接口
		foreach($request->header as $key=>$val) {
			echo "$key: $val\n";
		}
		var_dump($request);
		yield $response->end("hello world!");
	});
	$server->handle("/test2", function($request, $response) {
		$response->write_header(200);
		yield $response->end("hello world!");
	});
	$server->handle("/test3", function($req, $res) {
		$res->header["content-type"] = "text/plain";
		yield $res->write_file(__DIR__."/error.php");
	});
	$server->handle("/test4", function($req, $res) {

	});
	$server->handle_default(function($req, $res) {
		$res->write_header(404);
		yield $res->end("this is a error(404)");
	});
	echo "handler added\n";
	yield $server->listen_and_serve("0.0.0.0", 6676);
	echo "closed\n";
});
