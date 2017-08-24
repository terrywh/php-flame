<?php

flame\go(function() {

$obj = new flame\net\http\client([
	"debug"=>true, // 调试开关
]);
$obj->debug(1);

$req = new flame\net\http\client_request("https://www.google.com",["arg1"=>"123","arg2"=>"456"]);
$res = yield $obj->exec($req);
var_dump($res);

$req = new flame\net\http\client_request();
$req->method="GET";
$req->header = array("Accept"=>"123", "test"=>"Test");
$req->url = "www.google.com";
$req->timeout = 10;
$req->header["Accept"] = "123";
$req->body = ["arg1"=>"asd","arg2"=>"111qwe"];
$obj->debug(1);
$res = yield $obj->exec($req);
var_dump($res);

$req2 = new flame\net\http\client_request("www.google.com",["arg1"=>"123","arg2"=>"456"]);
$res2 = yield $obj->exec($req2);
var_dump($res2);

$res = yield flame\net\http\get("http://wiki.xingyan.pandatv.com:8090");
var_dump($res);

$res = yield flame\net\http\post("www.example.com", array("key"=>"123","value"=>"456"));
var_dump($res);

$res = yield flame\net\http\put("www.example.com", array("key"=>"123","value"=>"456"));
var_dump($res);

$req3 = new flame\net\http\client_request("www.baidu.com");
$res = yield $obj->exec($req3);
var_dump($res);

});

flame\run();
