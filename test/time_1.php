<?php
ob_start();

flame\init("time_1");
flame\go(function() {
	// 1. 毫秒时间戳
	assert(time() == intval(flame\time\now() / 1000));
	// 2. sleep
	$b = flame\time\now();
	yield flame\time\sleep(1000);
	$e = flame\time\now();
	assert( abs(1000 - ($e - $b)) < 10 ); // 大致准确
	// 3. timer
	$b = flame\time\now();
	flame\time\after(1000, function() use($b) {
		$e = flame\time\now();
		assert( abs(1000 - ($e - $b)) < 10 ); // 大致准确
		echo "done2.\n";
	});
	$t = new flame\time\timer(3000);
	$t->start(function($t) use($b) {
		$t->close();
		$e = flame\time\now();
		assert( abs(3000 - ($e - $b)) < 10 ); // 大致准确
		echo "done3.\n";
	});
	echo "done1.\n";
});
flame\run();

if(getenv("FLAME_PROCESS_WORKER")) {
	$output = ob_get_flush();
	// assert($output == "done1.\ndone2.\ndone3.\n");
}