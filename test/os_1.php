<?php
flame\init("os_1");
flame\go(function() {
	$info = flame\os\interfaces();
	var_dump($info);
	$proc = flame\os\spawn("ls", ["-l"], [
		"cwd" => "/data/htdocs",
	]);
	$proc->wait();
	var_dump( $proc->stdout() );
	
	echo "spawn: ping\n";
	$proc = flame\os\spawn("ping", ["-c", 5, "www.baidu.com"], [
		"cwd" => "/data/htdocs",
	]);
	flame\time\sleep(1000);
	$proc->wait();
	echo "exec: ping\n";

	$proc = flame\os\exec("ping", ["-c", 2, "www.baidu.com"]);
	var_dump($proc);
	unset($proc);
	
	for($i = 0; $i<10; ++$i) {
		flame\go(function() {
			for($j=0;$j<10;++$j) {
				flame\os\exec("ping", ["-c", 1, "www.baidu.com"]);
			}
		});
	}
	
	flame\time\sleep(1000);
	echo "done.\n";
});
flame\run();
