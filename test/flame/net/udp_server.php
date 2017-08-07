<?php
flame\go(function() {
	$server = new flame\net\udp_socket();
	$server->bind("::", 7678);
	while(true) {
		$addr = "";
		$port = 0;
		$data = yield $server->recv_from($addr, $port);
		echo "recv_from: ", $addr, ":", $port, " => [",$data, "]\n";
	}
});
flame\run();