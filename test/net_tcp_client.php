<?php
dl("flame.so");

flame\run(function() {
	echo "before create", "\n";
	$socket = new flame\net\tcp_socket();
	echo "before connect", "\n";
	yield $socket->connect("127.0.0.1", 6676);
	echo "local_addr: ", $socket->local_addr(), ":", $socket->local_port(), "\n";
	while(true) {
		$packet = "1-length[".rand()."]";
		yield $socket->write($packet);
		echo "<- ", $packet, "\n";
		// 读取方式 1. 读取指定长度的数据
		$packet = yield $socket->read(strlen($packet));
		echo "-> ", $packet, "\n";
		yield flame\sleep(100);
		$packet = "2-delim[".rand()."]\n";
		yield $socket->write($packet);
		echo "<- ", $packet;
		// 读取方式 2. 读取指定结束符标识的数据
		$packet = yield $socket->read("\n");
		echo "-> ", $packet;
		yield flame\sleep(300);
	}
	// close 过程可选，$socket 对象析构时会自动关闭
	// $socket->close();
});
