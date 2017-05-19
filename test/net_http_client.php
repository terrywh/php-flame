<?php
dl("flame.so");

flame\run(function() {
	$client = new flame\net\http\client();
	echo microtime(true),"\n";
	// 构造 request 对象
	$request = new flame\net\http\client_request("POST", "http://127.0.0.1:6676/test2");
	$request->header["xxxxx"] = aaaaa;
	$request->body = json_encode(["key"=>"val"]);
	// 通过 client 指定请求，并获得响应
	$response = yield $client->execute($request);
	var_dump($response);
	for($i=0;$i<100000;++$i) {
		// 简化请求方式
		$response = yield $client->get("http://127.0.0.1:6676/test2");
		// $response = yield $client->post("http://127.0.0.1:6676/test2", "post data here");
	}
	echo microtime(true),"\n";
});
