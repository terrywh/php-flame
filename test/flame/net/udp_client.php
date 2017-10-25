<?php
flame\init("udp_socket_test");
flame\go(function() {
	$sock = new flame\net\udp_socket();
	var_dump($sock);
	for($i=0;$i<10;++$i) {
		yield $sock->send("aaaaaaaaaaaaaaaaaa", "127.0.0.1", 7678);
		echo "->\n";
		yield flame\time\sleep(100);
	}
});
flame\run();
