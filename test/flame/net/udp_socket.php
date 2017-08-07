<?php
flame\go(function() {
	$sock = new flame\net\udp_socket();
	var_dump($sock);
	while(true) {
		yield $sock->send_to("aaaaaaaaaaaaaaaaaa", "127.0.0.1", 7678);
		yield flame\time\sleep(1000);
	}
});
flame\run();