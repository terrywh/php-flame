<?php
flame\init("ipc_worker_test");
flame\go(function() {
	flame\os\cluster\ondata(function($data) {
		echo "ondata: ", var_dump($data);
	});
	flame\os\cluster\handle(function($socket) {
		echo "handle: ", var_dump($socket);
		yield $socket->write("GET / HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n");
		var_dump(yield $socket->read());
	});
	// 发送给父进程（仅限 string）
	yield flame\os\cluster\send("this is a response");
	// 持续运行，等待被主进程杀死
	yield flame\time\sleep(500000);
});
flame\run();
