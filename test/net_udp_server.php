<?php
dl("flame.so");
// 启动 2 个子进程
// flame\fork(1);
// 共计 3 个进程会执行下面代码
flame\run(function() {
	$socket = new flame\net\udp_socket();
	// 接收来自指定端口的数据
	try{
		$socket->bind("0.0.0.0", 6676);
	}catch(Exception $ex) {
		echo $ex, "\n";
		exit;
	}
	echo "local_port: ", $socket->local_addr, ":", $socket->local_port, "\n";
	while(true) {
		$packet = yield $socket->read();
	 	echo "from: ", $socket->remote_addr(), ":", $socket->remote_port(), " => ", $packet, "\n";
	}
	// close 动作可选，对象销毁时会自动 close 底层的 socket
	// $socket->close();
});
