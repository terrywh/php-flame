<?php
flame\init("udp_socket_test");
flame\go(function() {
	$sock = new flame\net\udp_socket();
	var_dump($sock);
	while(true) {
		yield $sock->send("aaaaaaaaaaaaaaaaaa", "127.0.0.1", 7678);
		echo "->\n";
		yield flame\time\sleep(1000);
	}
});
flame\run();
