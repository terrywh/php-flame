<?php
dl("flame.so");
// 启动 2 个子进程
// flame\fork(1);
// 共计 3 个进程会执行下面代码
flame\run(function() {
	$server = new flame\net\udp_server();
	// 绑定接收来自指定端口的数据
	yield $server->bind("0.0.0.0", 6676);
	while(true) {
		$packet = yield $server->read();
		// remote_addr() / remote_port()
	 	echo "from: ", $server->remote_addr(), ":", $server->remote_port(), " => ", $packet, "\n";
	}
	// close 动作可选，对象销毁时会自动 close 底层的 socket
	// $socket->close();
});
