<?php
flame\init("unix_server_test");
flame\go(function() {
	$server = new flame\net\unix_server();
	// 每个连接会回调到单独的协程函数中进行处理
	$server->handle(function($sock) {
		var_dump($sock);
		$i = 0;
		while($i<1000) {
			try{
				yield $sock->write("".(++$i));
				yield flame\time\sleep(100);
			}catch(exception $ex) {
				// 客户端异常关闭连接会产生 BROKEN_PIPE 等错误需要处理
				var_dump($ex);
				break;
			}
		}
	});
	@unlink("/data/sockets/flame.xingyan.panda.tv.sock");
	$server->bind("/data/sockets/flame.xingyan.panda.tv.sock");
	// 在 $server->close() 之前，会“阻塞”在此处
	yield $server->run();
	echo "======\n";
});
flame\run();
