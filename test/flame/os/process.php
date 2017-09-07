<?php
flame\go(function() {
	$proc = flame\os\start_process("/usr/bin/ping", ["www.baidu.com"], [
		"PATH"=>"/usr/bin",
		"ENV_KEY_1"=>"ENV_VAL_1"
	], "/tmp", [
		"gid" => 2017, "uid" => 2017,
		"stdout" => "/tmp/ping.log",
	]);
	echo "--------\n";
	yield flame\time\sleep(50000);
	echo "--------\n";
	$proc->kill();
});
flame\run();
