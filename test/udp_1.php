<?php
ob_start();

flame\init("tcp_1");

flame\go(function() {
	$server = new flame\udp\socket("127.0.0.1:17678");
	$from = "";
	$data = yield $server->receive_from($from);
	assert($data == "aaa\r\naaa");
	assert(substr($from, 0, 10) == "127.0.0.1:");
	echo "done1.\n";
});
flame\go(function() {
	// 等待服务端启动完毕
	yield flame\time\sleep(2000);
	$socket = yield flame\udp\connect("127.0.0.1:17678");
	yield $socket->send("aaa\r\naaa");
	yield $socket->send_to("bbbbbb", "127.0.0.1:17679");
	echo "done2.\n";
});
flame\go(function() {
	$server = new flame\udp\socket("127.0.0.1:17679");
	$from = "";
	$data = yield $server->receive_from($from);
	assert($data == "bbbbbb");
	assert(substr($from, 0, 10) == "127.0.0.1:");
	echo "done3.\n";
});
flame\run();

if(getenv("FLAME_PROCESS_WORKER")) {
	$output = ob_get_flush();
	assert($output == "done1.\ndone2.\ndone3.\n");
}