<?php
flame\init("process_test");
// flame\go(function() {
// 	$proc = flame\os\spawn("/usr/bin/ping", ["www.baidu.com"], [
// 		"PATH"=>"/usr/bin",
// 		"ENV_KEY_1"=>"ENV_VAL_1"
// 	], "/tmp", [
// 		"gid" => 2017, "uid" => 2017,
// 		"stdout" => "/tmp/ping.log",
// 	]);
// 	echo "--------\n";
// 	yield flame\time\sleep(50000);
// 	echo "--------\n";
// 	$proc->kill();
// });
flame\go(function() {
	$proc = new flame\os\process(flame\os\executable(),
		[__DIR__."/worker.php"],
		null,
		__DIR__,
		[
			"ipc" => true,
		]);
	echo "=========\n";
	yield flame\time\sleep(5000);
	echo "=========\n";
	yield $proc->send("aaaaaaaaaaaaa");
	yield flame\time\sleep(50000);
	echo "=========\n";
	$proc->kill();
});
flame\run();
