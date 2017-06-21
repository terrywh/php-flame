<?php
dl("flame.so");

flame\run(function() {
	$client = new flame\http\client();
	echo microtime(true),"\n";
	// 构造 request 对象
    for($i = 0; $i < 50; $i++) {
    //$i = 0;
        $request = new flame\http\client_request("GET", "https://www.baidu.com/");
        $request->header["Host"] = "www.baidu.com";
        //$request->body = json_encode(["key"=>"val"]);
        // 通过 client 指定请求，并获得响应
        $response = yield $client->execute($request);
        var_dump($i.' => '.$response->status);
    }
    //var_dump($response);
    yield flame\sleep(20000);
    for($i = 0; $i < 50; $i++) {
        $request = new flame\http\client_request("GET", "https://www.sogou.com/");
        $request->header["Host"] = "www.sogou.com";
        //$request->body = json_encode(["key"=>"val"]);
        // 通过 client 指定请求，并获得响应
        $response = yield $client->execute($request);
        var_dump($i.' => '.$response->status);
    }
    yield flame\sleep(20000);

    /*$request = new flame\http\client_request("GET", "https://www.baidu.com/");*/
    //$request->header["Host"] = "www.baidu.com";
    //$response = yield $client->execute($request);
    /*var_dump('after free => '.$response->status);*/
	echo microtime(true),"\n";
});
