<?php
flame\init("spawn_ipc_master");
flame\go(function() {
	// 简单启动进程，并获取输出
	// $data = yield flame\os\exec("ps", ["-ef"]);
	// var_dump($data);
	
	var_dump( flame\os\executable() );
	$proc = new flame\os\process(
		flame\os\executable(),
		[__DIR__."/spawn_worker.php"],
		[
			"env"    => ["TEST_KEY_1" => "TEST_VAL_1"],
			"cwd"    => __DIR__,
			"stdout" => "pipe", // 重定向输出管道
			"ipc" => true,
		]);
	// 在单独的协程中接收进程标准输出
	flame\go(function() use($proc) {
		var_dump( yield $proc->stdout()->read_all() );
	});
	// 接收来自子进程 IPC 管道的通讯数据
	$proc->ondata(function($data) {
		echo "master ondata: ", var_dump($data);
	});	
	yield flame\time\sleep(2000);
	echo "before sending (1)...\n";
	yield $proc->send("this is a string message");
	echo "after sending (1)...\n";
	yield flame\time\sleep(2000);
	echo "before sending (2)...\n";
	$sock = new flame\net\tcp_socket();
	yield $sock->connect("11.22.33.44", 80);
	yield $proc->send($sock); // send to child process
	yield flame\time\sleep(5000);
	echo "kill worker ...\n";
	$proc->kill();
});
flame\run();
