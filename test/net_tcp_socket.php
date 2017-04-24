<?php
dl("mill.so");

$socket = new mill\net\tcp_socket("127.0.0.1", 6676);
echo "remote_addr: ", $socket->remote_addr(), "\n";
while(true) {
	$n = $socket->send("12345678\n");
	$socket->flush();
	$packet = $socket->recv($n);
	echo "=> ", $packet;
	$socket->send_now("aaaaaaaaaaa\n");
	$packet = $socket->recv("\n");
	echo "=> ", $packet;
	mill\sleep(4000);
}

$socket->close();
