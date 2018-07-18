<?php
flame\init("os_2");
flame\go(function() {
	ob_start();
	$proc = flame\os\spawn("ping", ["www.baidu.com"]);
	flame\time\after(5000, function() use($proc) {
		$proc->kill();
	});
	yield $proc->wait();
	assert( count(explode("\n", $proc->stdout())) > 5 );

	$output = yield flame\os\exec("ls");
	assert( count(explode("\n", $output)) >= 6 );
	echo "done1.\n";
	$output = ob_get_flush();
	assert($output == "done1.\n");
});
flame\run();
