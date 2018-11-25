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
	var_dump( flame\os\exec("ping", ["-c", 3, "www.baidu.com"]) );
});
flame\run();
