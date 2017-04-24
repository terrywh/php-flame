<?php
dl("mill.so");

$server = new mill\net\tcp_server("127.0.0.1", 6676);
echo "local_port: ", $server->local_port, "\n";
while(true) {
	$socket = $server->accept();
	echo "accept: ", $socket->remote_addr(), "\n";
	// 启动协程，不阻塞 accept 过程
	mill\go(function() use($socket) {
		do {
			$request = mill\http\parse_request($socket);
			// $chunked = false, $gzip = false
			$response = $request->build_response();
			$response->send("aaaa");
			$response->send("bbbb");
			$response->end("ccccccccccccc");
			// $request->header / body
			mill\sleep(1000);
		}while($request->header["connection"] != "keep-alive");
		$socket->close();
	});
}
$server->close();
