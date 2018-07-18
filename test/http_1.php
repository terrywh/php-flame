<?php
flame\init("http_1");
flame\go(function() {
	ob_start();
	$req = new flame\http\client_request("http://www.baidu.com/abc.html?a=b&c=d", ["e"=>"f", "g"=>"h"], 3000);
	$req->header["Content-Type"] = "application/json";
	$res = yield flame\http\exec($req);
	assert($res->status == 302);
	$res = yield flame\http\get("https://www.baidu.com/");
	assert(intval($res->header["content-length"]) > 14000);
	echo "done1.\n";
	$output = ob_get_flush();
	assert($output == "done1.\n");
});
flame\run();
