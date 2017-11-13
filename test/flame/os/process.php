<?php
flame\init("ipc_master_test");
flame\go(function() {
	flame\os\cluster\ondata(function($data) {
		echo "ondata: ", var_dump($data);
	});
	$proc = new flame\os\process(flame\os\executable(),
		[__DIR__."/worker.php"],
		null,
		__DIR__,
		[
			"ipc" => true,
		]);
	yield flame\time\sleep(2000);
	yield $proc->send("this is a string message");
	yield flame\time\sleep(2000);
	$sock = new flame\net\tcp_socket();
	yield $sock->connect("127.0.0.1", 80);
	yield $proc->send($sock); // send to child process
	yield flame\time\sleep(2000);
	echo "=========\n";
	$proc->kill();
});
flame\run();
