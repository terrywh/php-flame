<?php
flame\init("ipc_master_test");
flame\go(function() {
	$proc = new flame\os\process(flame\os\executable(),
		[__DIR__."/worker.php"],
		null,
		__DIR__,
		[
			"ipc" => true,
		]);
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
	$proc->kill();
});
flame\run();
