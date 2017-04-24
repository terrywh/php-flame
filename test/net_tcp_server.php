<?php
dl("mill.so");

$server = new mill\net\tcp_server("127.0.0.1", 6676);
echo "local_port: ", $server->local_port, "\n";
while(true) {
	$socket = $server->accept();
	echo "accept: ", $socket->remote_addr(), "\n";
	// 启动协程，不阻塞 accept 过程
	mill\go(function() use($socket) {
		$data = $socket->recv("\n");
		mill\sleep(1000);
		$socket->send_now($data);
		mill\sleep(1000);
		$socket->close();
	});
}
$server->close();
