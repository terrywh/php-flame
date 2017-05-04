<?php
dl("flame.so");

flame\run(function() {
	$server = new flame\net\tcp_server();
	$server->listen("127.0.0.1", 6676);
	while(true) {
		$socket = yield $server->accept();
		// 启动“协程”，不阻塞 accept 过程
		flame\go(function() use($socket) {
            while(true) {
                echo "request ......\n";
                //throw new Exception("aaaaa");
                try {
                $req = yield flame\net\http\request::parse($socket);
                } catch (exception $ex) {
                    var_dump($ex);
                    //throw $ex;
                    break;
                }
                echo "process complete ......\n";
                var_dump($req);
                //var_dump($req->head["cookie"]);
            }
		});
	}
});

