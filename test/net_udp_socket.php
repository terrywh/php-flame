<?php
dl("mill.so");

$socket = new mill\net\udp_socket("127.0.0.1", 6676);
echo "local_port: ", $socket->local_port, "\n";
while(true) {
	$packet = $socket->recv();
	$addr = $socket->remote_addr();
	echo "from: ", $addr, " => ", $packet, "\n";
	$socket->send($packet, "127.0.0.1", 6677);
}

$socket->close();
