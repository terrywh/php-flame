<?php
dl("mill.so");

$server = new mill\net\tcp_server("0.0.0.0", 6676);
echo "local_port: ", $server->local_port, "\n";
$socket = [];
$i = 0;
while(true) {
	$socket = $server->accept();
	// echo "accept: ", $socket->remote_addr(), "\n";
	// 启动协程，不阻塞 accept 过程
	mill\go(function() use($socket) {
		echo "i=$i\n";
		var_dump($socket[$i-1]);
		// do {
			try{
		 		$request = mill\http\request::parse($socket[$i-1], 5000);
			}catch(exception $ex) {
				var_dump($ex);
			}
		// 	// var_dump($request);
		// 	$socket[$i]->send("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/plain\r\nContent-Length: 12\r\n\r\nhello world!");
		// }while(true);
		$socket[$i-1]->close();
	});
}
$server->close();
