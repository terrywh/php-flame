<?php
flame\init("spawn_ipc_worker");
flame\go(function() {
	flame\os\cluster\ondata(function($data) {
		// echo "worker ondata: ", var_dump($data);
		// 发送给父进程（仅限 string）
		yield flame\os\cluster\send("this is a response");
	});
	flame\os\cluster\handle(function($socket) {
		// echo "worker handle: ", var_dump($socket);
		yield $socket->write("GET / HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n");
		echo yield $socket->read(), "\n";
		//echo "worker read: ", var_dump(yield $socket->read());
	});
	// 主进程将捕获下面输出：
	echo "aaaaaaaaaaaaa\n";
	echo "bbbbbbbbbbbbb\n";
	// 持续运行，等待被主进程杀死
	yield flame\time\sleep(500000);
});
flame\run();
