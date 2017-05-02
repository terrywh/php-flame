<?php
dl("flame.so");
flame\run(function() {
	$socket = new flame\net\udp_socket();
	// "连接" 指定地址发送内容，注意 connect 后不能调用 write_to 函数
	yield $socket->connect("127.0.0.1", 6676);
	echo "local_addr: ", $socket->local_addr, ":", $socket->local_port, "\n";
	while(true) {
		try{
			// connect 方式 write 可能发生异常 connection refused
			yield $socket->write("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
			// 上面 connect + write 可以考虑替换为：
			// yield $socket->write_to("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "127.0.0.1", 6676);
		}catch(Exception $e) {
			var_dump($e);
		}
		yield flame\sleep(10);
	}
	// close 操作可选，析构时会自动清理
	// $socket->close();
});
