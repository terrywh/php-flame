<?php
flame\init("tcp_client_test");
flame\go(function() {
	$sock = new flame\net\tcp_socket();
	yield $sock->connect("10.20.6.75", 80);
	var_dump($sock);
	yield $sock->write("GET / HTTP/1.1\r\nHost: online.panda.tv\r\n\r\n");
	while(true) {
		$data = yield $sock->read("\r\n");
		if($data === null) { // 连接被对方关闭（EOF）
			break;
		}
		echo "=> ", $data, "\n";
		
		$data = yield $sock->read(2);
		if($data === null) { // 连接被对方关闭（EOF）
			break;
		}
		echo "=> ", $data, "\n";
	}
});
flame\run();
