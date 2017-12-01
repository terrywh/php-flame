<?php
flame\init("spawn_ipc_master");
flame\go(function() {
	// 简单启动进程，并获取输出
	yield flame\os\exec("ps", ["-ef"])
	 
	$proc = new flame\os\process(
		flame\os\executable(),
		[__DIR__."/worker.php"],
		[
			"env"    => ["TEST_KEY_1" => "TEST_VAL_1"],
			"cwd"    => __DIR__,
			"stdout" => "pipe", // 重定向输出管道
			"ipc" => true,
		]);
	// 在单独的协程中接收进程标准输出
	flame\go(function() use($proc) {
		$data = yield $proc->stdout()->read_all();
		var_dump($data);
	});
	// 接收来自子进程 IPC 管道的通讯数据
	$proc->ondata(function($data) {
		echo "master ondata: ", var_dump($data);
	});
	yield flame\time\sleep(2000);
	yield $proc->send("this is a string message");
	yield flame\time\sleep(2000);
	$sock = new flame\net\tcp_socket();
	yield $sock->connect("10.20.6.75", 80);
	yield $proc->send($sock); // send to child process
	yield flame\time\sleep(5000);
	echo "kill worker ...\n";
	$proc->kill();
});
flame\run();
