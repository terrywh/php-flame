<?php
dl("flame.so");
// 启动 2 个子进程
// flame\fork(1);
// 共计 3 个进程会执行下面代码
flame\run(function() {
	$server = new flame\net\udp_server();
	// 接收来自指定端口的数据
	try{
		$server->bind("0.0.0.0", 6676);
	}catch(Exception $ex) {
		echo $ex, "\n";
		exit;
	}
	while(true) {
		$packet = yield $server->read();
		// remote_addr() / remote_port() 与 local_* 不同，前者为方法，后者为属性
	 	echo "from: ", $server->remote_addr(), ":", $server->remote_port(), " => ", $packet, "\n";
	}
	// close 动作可选，对象销毁时会自动 close 底层的 socket
	// $socket->close();
});
