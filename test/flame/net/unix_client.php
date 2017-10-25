<?php
flame\init("unix_socket_test");
flame\go(function() {
	$sock = new flame\net\unix_socket();
	yield $sock->connect("/data/sockets/flame.xingyan.panda.tv.sock");
	var_dump($sock);
	while(true) {
		$data = yield $sock->read(2);
		if($data === null) { // 连接被对方关闭（EOF）
			break;
		}
		echo strlen($data),"=> ", $data, "\n";
	}
});
flame\run();
