<?php
dl("flame.so");
flame\run(function() {
	$socket = new flame\net\udp_socket();
	// "连接" 指定地址发送内容，注意 connect 后不能调用 write_to 函数
	$socket->connect("127.0.0.1", 6676);
	while(true) {
		try{
			// connect 方式 write 可能发生异常 connection refused
			yield $socket->write("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
		}catch(Exception $e) {
			var_dump($e);
		}
		yield flame\sleep(5000);
	}
	// close 操作可选，析构时会自动清理
	// $socket->close();
});
