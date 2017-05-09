<?php
dl("flame.so");

flame\run(function() {
	$server = new flame\net\tcp_server();
	$server->listen("127.0.0.1", 6676);
	while(true) {
		$socket = yield $server->accept();
		// 启动“协程”，不阻塞 accept 过程
		flame\go(function() use($socket) {
            while($req = yield flame\net\http\request::parse($socket)) {
                echo "request ......\n";
                //throw new Exception("aaaaa");
                //try {
                /*} catch (exception $ex) {*/
                    //var_dump($ex);
                    //break;
                /*}*/
                var_dump($req);
                //var_dump($req->header);
                //var_dump($req->body);
                $body = "aaaaa\n";
                echo "request complete ......\n";
                $rsp = yield flame\net\http\response::build($req);
                $rsp->header["content-length"] = strlen($body);
                $rsp->header["connection"] = "close";
                //$rsp->header["content-length"] = "5";
                var_dump($rsp);
                //yield $rsp->write_header(200);
                yield $rsp->write($body);
                yield $rsp->end();
                //unset($rsp);
                //unset($req);
                //break;
            }
            //unset($socket);
		});
	}
});

