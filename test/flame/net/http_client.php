<?php
flame\init("http_client_test");

flame\go(function() {
	// $cli = new flame\net\http\client([
	// //	"debug"=>true, // 调试开关
	// ]);
	//
	// $req = new flame\net\http\client_request("http://www.baidu.com");
	// $res = yield $cli->exec($req);
	// var_dump($res);
	// echo $res; // 自动转换文本
	//
	//
	// $req = new flame\net\http\client_request();
	// $req->method  = "GET";
	// $req->header  = array("Accept"=>"123", "test"=>"Test");
	// $req->url     = "http://www.baidu.com";
	// $req->timeout = 2000;
	// $req->header["Accept"] = "123";
	// $req->body = ["arg1"=>"asd","arg2"=>"111qwe"];
	// $res = yield $cli->exec($req);
	// // var_dump($res);
	//
	// $res = yield flame\net\http\post("http://www.baidu.com", array("key"=>"123","value"=>"456"));
	// // var_dump($res);
	$data = [
	    "from" => [
	        "rid"  => 123456,
	        "nick" => "昵称1"
	    ],
	    "to"   => 654321,
	    "type" => "site",
	    "data" => [
	        "gift_id"  => "59ef2e56af221943d545edc2",
	        "gift_name"=> "英雄令",
	        "combo"    => 9999,
	        "host_nick"=> "主播昵称",
	        "subtitle_content" => ""
	    ]
	];
	$data = json_encode($data, JSON_UNESCAPED_UNICODE);
	$room = 654321;
	$user = 0;
	$cmd  = 2007;
	$res = yield flame\net\http\post("http://senderb.xingyan.pdtv.io:8360/notify/room",
		"[{$room}, {$user}, 0, {$cmd}, {$data}]\n");
	var_dump($res);
});

flame\run();
