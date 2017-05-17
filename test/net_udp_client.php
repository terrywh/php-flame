<?php
dl("flame.so");
flame\run(function() {
	$socket = new flame\net\udp_socket();
	echo "created\n";
	// "连接" 指定地址发送内容，注意 connect 后不能调用 write_to 函数
	yield $socket->connect("127.0.0.1", 6676);
	echo "connected: ", $socket->local_port(), "=>", $socket->remote_port(), "\n";
	for($i=0;$i<10000;++$i) {
		// yield $socket->write_to("aaaaaa", "127.0.0.1", 6677);
		// 使用 connect 后，可直接使用下面代替上面 write_to
		yield $socket->write("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
		echo "-> aaaaaa\n";
		yield flame\sleep(10);
	}
	// close 操作可选，析构时会自动清理
	// $socket->close();
});
