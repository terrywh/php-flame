<?php
dl("flame.so");
// flame\fork(1);
flame\run(function() {
	$server = new flame\net\tcp_server();
	$server->listen("0.0.0.0", 6676);
	while(true) {
		$socket = yield $server->accept();
		// 启动“协程”，不阻塞 accept 过程
		flame\go(function() use($socket) {
			try{
				while($req = yield flame\net\http\request::parse($socket)) {
					$res = flame\net\http\response::build($req);
					// $response 仅支持 Transfer-Encoding: chunked 方式
					// 请不要改变这一逻辑，例如，不要 不要 做下面的代码：
					// 
					// 	$res->header["Content-Length"] = 12;
					// 	unset($res->header["Transfer-Encoding"]);
					yield $res->write_header(200);
					yield $res->end("hello world!");
				}
			}catch(exception $e) {
				echo "error: ", $e->getMessage(),"\n";
			}
		});
	}
});
