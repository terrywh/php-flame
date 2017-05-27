<?php
dl("flame.so");

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
