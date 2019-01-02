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

	$proc = flame\os\spawn("ping", ["-c", 5, "www.baidu.com"], [
		"cwd" => "/data/htdocs",
	]);
	flame\time\sleep(1000);
	$proc->wait();
	
	
	for($i = 0; $i<10; ++$i) {
		flame\go(function() {
			for($j=0;$j<100;++$j) {
				flame\os\exec("ping", ["-c", 1, "www.baidu.com"]);
			}
		});
	}
	flame\time\sleep(60000);
	echo "done.\n";
});
flame\run();
