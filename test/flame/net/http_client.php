<?php
flame\init("http_client_test");

flame\go(function() {
	// 自行构建客户端
	$cli = new flame\net\http\client([
		"conn_per_host" => 4,
	]);
	// 创建请求
	$req = new flame\net\http\client_request("http://127.0.0.1:7678/test/path");
	// 整体设置 HEADER
	// $req->header  = [
	// 	"apns-topic" => "aaaaaa",
	// ];
	// 也可以单独设置
	$req->header["apns-topic"] = "aaaaa";
	// 客户端证书设置
	// $req->ssl([
	// 	"cert" => "/tmp/test_cert.pem",
	// 	"key"  => "/tmp/test_key.pem",
	// 	"pass" => "123456",
	// ]);
	$res = $cli->exec($req);
	var_dump($res);
	// 简化函数
	$res = yield flame\net\http\get("http://www.baidu.com/");
	var_dump($res);
});

flame\run();
