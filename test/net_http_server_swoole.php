<?php
dl("swoole.so");

$http = new swoole_http_server("0.0.0.0", 6676);
$http->set(array(
    'reactor_num' => 2, //reactor thread num
    'worker_num' => 1,    //worker process num
    'backlog' => 1024,   //listen backlog
    'max_request' => 0,
    'dispatch_mode' => 1,
));
$http->on('request', function ($request, $response) {
	$response->status(200);
    $response->end("hello world!");
});
$http->start();
