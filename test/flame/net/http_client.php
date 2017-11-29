<?php
flame\init("http_client_test");

flame\go(function() {
	// 自行构建客户端
	$cli = new flame\net\http\client([
		"conn_share" => "plex",
		"conn_per_host" => 4,
	]);
	for($i = 0;$i<10; ++$i) {
		// 创建请求
		flame\go(function() use($cli, $i) {
			$req = new flame\net\http\client_request("http://www.baidu.com/");
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
			echo $i,"\n";
		});
		yield flame\time\sleep(100);
	}
	var_dump($res);
	// 简化函数
	$res = yield flame\net\http\get("http://www.baidu.com/");
	var_dump($res);
	
	echo "done\n";
});

flame\run();
