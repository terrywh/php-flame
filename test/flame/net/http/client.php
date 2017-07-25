<?php
flame\go(function() {
$obj = new flame\net\http\client();
$req = new flame\net\http\request(["_POST"=>["arg1"=>"123","arg2"=>"456"],"url"=>"www.google.com","method"=>"POST"]);
//$req->header = array("Accept"=>"123", "test"=>"Test");
//var_dump($req->header);
//echo "Method:", $req->method, "\n";
//$req->url = "www.baidu.com";
//$req->timeout = 1;
//$req->header["Accept"] = "123";
$obj->debug(1);
$res = yield $obj->exec($req);
var_dump($obj);
var_dump($res);
//$res2 = yield $obj->exec($req);
//var_dump($res2);

//$res = yield flame\net\http\get("www.google.com");
//var_dump($res);
$res = yield flame\net\http\post("www.example.com", array("key"=>"123","value"=>"456"));
var_dump($res);

//$req = new flame\net\http\request("www.baidu.com");
//$res = $obj->exec($req);
//var_dump($res);

//$req = new flame\net\http\request("POST", "www.google.com", array("key"=>"4123", "value"=>"4321"));
//$res = $obj->exec($req);
//var_dump($res);

});
flame\run();
