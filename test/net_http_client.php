<?php
dl("flame.so");

flame\run(function() {
	$client = new flame\net\http\client();

	$request = new flame\net\http\client_request("GET", "http://bullet.xingyan.panda.tv/room/user?xid=4232544&hostid=5508152&limit=200");
	// $request->header["Connection"] = "Keep-Alive";
	// $request->data = "aaaaaaaaaaaaa";   // -> content-type: text/plain
	// $request->data = ["aaa"=>"bbbbbb"]; // -> content-type: application/x-www-form-urlencoded

	$response = yield $client->execute($request);
	var_dump($response);
	// $response = yield $agent->get($request)
	// if($response->statusCode == 200) {
	// 	echo "response header:", var_export($response->header, true), "\n"
	// 	echo "response body:", $response->body(), "\n";
	// }else{
	// 	echo "response failed:", $response->statusCode , "\n";
	// }
});
