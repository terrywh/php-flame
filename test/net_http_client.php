<?php
dl("flame.so");

flame\run(function() {
	$agent = new flame\net\http\agent();

	$request = new flame\net\http\client_request("http://www.baidu.com/");
	$request->header["Connection"] = "Keep-Alive";
	$request->data = "aaaaaaaaaaaaa";   // -> content-type: text/plain
	$request->data = ["aaa"=>"bbbbbb"]; // -> content-type: application/x-www-form-urlencoded

	$response = yield $agent->post($request);
	// $response = yield $agent->get($request)
	if($response->statusCode == 200) {
		echo "response header:", var_export($response->header, true), "\n"
		echo "response body:", $response->body(), "\n";
	}else{
		echo "response failed:", $response->statusCode , "\n";
	}
});
