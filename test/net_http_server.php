<?php
dl("mill.so");

$server = new mill\net\tcp_server("0.0.0.0", 6676);
echo "local_port: ", $server->local_port, "\n";
while(true) {
	$socket = $server->accept();
	// echo "accept: ", $socket->remote_addr(), "\n";
	// 启动协程，不阻塞 accept 过程
	mill\go(function() use($socket) {
		do {
			$request = mill\http\request::parse($socket);
			// var_dump($request->header);
			var_dump($request);
		// 	// $request->header / body
		// 	mill\sleep(100);
		// 	// // $gzip = false
		// 	// $response = $request->build_response();
		// 	// $response->send("aaaa");
		// 	// $response->send("bbbb");
		// 	// $response->end("ccccccccccccc");
		//
			$socket->send("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/plain\r\nContent-Length: 12\r\n\r\nhello world!");
		}while(false);
		$socket->close();
	});
}
$server->close();
