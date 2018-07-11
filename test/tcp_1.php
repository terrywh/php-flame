<?php
ob_start();

flame\init("tcp_1");
flame\go(function() {
	// 等待服务端启动完毕
	yield flame\time\sleep(1000);
	$socket = yield flame\tcp\connect("127.0.0.1:7678");
	yield $socket->write("aaaaaa\r\nbbb");
	yield $socket->write("bbbcccccc");
	echo "done1.\n";
});
flame\go(function() {
	$server = new flame\tcp\server("127.0.0.1:7678");
	yield $server->run(function($socket) use($server) {
		$data = yield $socket->read("\r\n");
		assert($data == "aaaaaa\r\n");
		$data = yield $socket->read(6);
		assert($data == "bbbbbb");
		$server->close();
		echo "done2.\n";
	});
	echo "done3.\n";
});
flame\run();

if(getenv("FLAME_PROCESS_WORKER")) {
	$output = ob_get_flush();
	assert($output == "done1.\ndone2.\ndone3.\n");
}