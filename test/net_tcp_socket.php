<?php
dl("flame.so");

flame\run(function() {
	$socket = new flame\net\tcp_socket();
	yield $socket->connect("127.0.0.1", 6676);
	echo "local_addr: ", $socket->local_addr, ":", $socket->local_port, "\n";
	while(true) {
		$packet = "method-length[".rand()."]";
		echo "<- ", $packet, "\n";
		$n = yield $socket->write($packet);
		// 读取方式 1. 读取指定长度的数据
		$packet = yield $socket->read($n);
		echo "-> ", $packet, "\n";
		yield flame\sleep(1000);
		$packet = "method-delim[".rand()."]\n";
		echo "<- ", $packet;
		yield $socket->write($packet);
		// 读取方式 2. 读取指定结束符标识的数据
		$packet = yield $socket->read("\n");
		echo "-> ", $packet;
		yield flame\sleep(3000);
	}
	// close 过程可选，$socket 对象析构时会自动关闭
	// $socket->close();
});
