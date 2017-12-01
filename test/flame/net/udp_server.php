<?php
flame\init("udp_server_test");
flame\go(function() {
	$server = new flame\net\udp_socket();
	$server->bind("::", 7678);
	while(true) {
		$data = yield $server->recv();
		echo "data: ", $data, " <= ", $data->remote_address, ":", $data->remote_port, "\n";
	}
});
flame\run();
