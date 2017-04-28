<?php
dl("flame.so");
flame\run(function() {
	$socket = new flame\net\udp_socket();
	// 接收来自指定端口的数据
	$socket->bind("0.0.0.0", 6676);
	echo "local_port: ", $socket->local_addr, ":", $socket->local_port, "\n";
	while(true) {
		$packet = yield $socket->read();
	 	echo "from: ", $socket->remote_addr(), ":", $socket->remote_port(), " => ", $packet, "\n";
	}
	// close 动作可选，对象销毁时会自动 close 底层的 socket
	// $socket->close();
});
