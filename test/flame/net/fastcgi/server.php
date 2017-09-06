<?php
// 可选，初始化框架配置
// flame\init("fastcgi-server", [
// 	"worker" => 2, // 启动两个工作进程，默认 0
// ]);
flame\go(function() {
	// 创建 fastcgi 服务器
	$server = new flame\net\fastcgi\server();
	// 设置处理程序
	$server->handle("/hello", function($req, $res) {
		yield flame\time\sleep(2000);
		var_dump($req->method, $req->body); // GET POST PUT ....
		yield $res->write("hello ");
		yield $res->end("world\n");
	})
	->handle("/favicon.ico", function($req, $res) {
		yield $res->write_header(404);
		yield $res->end();
	})
	// 默认处理程序（即：不匹配上述路径形式时调用）
	->handle(function($req, $res) {
		yield flame\time\sleep(2000);
		var_dump($req);
		$data = json_encode($req);
		yield $res->end($data);
	});
	// 1. 绑定 unix domain socket 理论性能更高（注意 fastcgi_keep_conn on; 配置）
	$server->bind("/data/sockets/flame.xingyan.panda.tv.sock");
	@chmod("/data/sockets/flame.xingyan.panda.tv.sock", 0777);
	// 方式2. 绑定 tcp 网络，理论上在并发连接数高（>1000）时较好
	// $server->bind("127.0.0.1", 19001);
	yield $server->run();
});
flame\run();
