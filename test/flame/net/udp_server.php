<?php
flame\init("udp_server_test");
flame\go(function() {
	$server = new flame\net\udp_socket();
	$server->bind("::", 7678);
	while(true) {
		$addr = "";
		$port = 0;
		$data = yield $server->recv($addr, $port);
		echo "recv: ", $addr, ":", $port, " => [",$data, "]\n";
	}
});
flame\run();
