<?php
flame\init("http_client_test");

flame\go(function() {
	$req = new flame\net\http\client_request("http://ip.cn/", null);
	$req->header["User-Agent"] = "curl/7.52.1";
	$res = yield flame\net\http\exec($req); // 用默认客户端执行请求
	var_dump($res->body);
	// 自行构建客户端
	$cli = new flame\net\http\client([
		"conn_share" => "plex",
		"conn_per_host" => 4,
	]);
	$res = yield $cli->get("http://ip.cn/");
	var_dump($res->status);
	for($i = 0;$i<50; ++$i) {
		// 创建请求
		flame\go(function() use($cli, $i) {
			$req = new flame\net\http\client_request("http://www.baidu.com/");
			$req->header["User-Agent"] = "curl/7.52.1";
			// 整体设置 HEADER
			// $req->header  = [
			//  	"apns-topic" => "aaaaaa",
			// ];
			// 也可以单独设置
			// $req->header["apns-topic"] = "aaaaa";
			// 客户端证书设置
			// $req->ssl([
			// 	"cert" => "/tmp/test_cert.pem",
			// 	"key"  => "/tmp/test_key.pem",
			// 	"pass" => "123456",
			// ]);
			$res = yield $cli->exec($req);
			echo $i," -> ", $res->status, "\n";
		});
		yield flame\time\sleep(200);
	}
	var_dump($res->status);
	// 简化函数
	$res = yield flame\net\http\get("http://www.baidu.com/"); // 构造请求并用默认客户端执行
	var_dump($res->status);
	$res = yield flame\net\http\post("http://www.baidu.com/", ["a"=>"b"]);
	var_dump($res->status);
	$res = yield flame\net\http\post("http://www.baidu.com/", json_encode(["a"=>"b"]));
	var_dump($res->status);
	$res = yield flame\net\http\post("http://www.baidu.com/", []);
	var_dump($res->status);
	$res = yield flame\net\http\post("http://www.baidu.com/", null);
	var_dump($res->status);
	$res = yield flame\net\http\post("http://www.baidu.com/", "");
	var_dump($res->status);
	$res = yield flame\net\http\post("http://www.baidu.com/", "aaaaa");
	var_dump($res->status);
	echo "done\n";
});

flame\run();
