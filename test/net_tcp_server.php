<?php
dl("mill.so");
function main() {
	$server = new mill\net\tcp_server("127.0.0.1", 6676);
	echo "local_port: ", $server->local_port, "\n";
	$socket = null;

	while(true) {
		$socket = yield $server->accept();
		// echo "accept: ", $socket->remote_addr(), "\n";
		var_dump(1,$socket);
		// 启动协程，不阻塞 accept 过程
		mill\go(function() use($socket) {
			$socket2 = $socket;
			var_dump(2, $socket2);
			// $data = $socket->recv("\n");
			yield mill\sleep(10000);
			// $socket->send_now($data);
			// mill\sleep(1000);
			// echo "close: ", $socket->remote_addr(), "\n";
			var_dump(3,$socket2);
			$socket2->close();
		});
		unset($socket);
	}

	$server->close();
}

run(main);
