<?php
dl("flame.so");

flame\run(function() {
	$server = new flame\net\tcp_server();
	$server->listen("127.0.0.1", 6676);
	while(true) {
		$socket = yield $server->accept();
		// 启动“协程”，不阻塞 accept 过程
		flame\go(function() use($socket) {
			while(true) {
				$packet = yield $socket->read("]");
				echo $socket->remote_addr(), ":", $socket->remote_port(), " -> ", $packet, "\n";
				yield $socket->write($packet);
				echo "-> ", $packet, "\n";
				$packet = yield $socket->read("\n");
				echo $socket->remote_addr(), ":", $socket->remote_port(), " -> ", $packet;
				yield $socket->write($packet);
				echo "-> ", $packet;
			}
		});
	}
	// $server->close();
	// close 可选，对象会自动销毁
});
