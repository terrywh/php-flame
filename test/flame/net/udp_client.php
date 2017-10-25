<?php
flame\init("udp_socket_test");
flame\go(function() {
	$sock = new flame\net\udp_socket();
	var_dump($sock);
	for($i=0;$i<1000;++$i) {
		yield $sock->send("".$i, "127.0.0.1", 7678);
		echo "-> ", $i, "\n";
		yield flame\time\sleep(100);
	}
});
flame\run();
