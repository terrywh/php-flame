<?php
flame\init("time_ticker");
flame\go(function() {
	$count = 0;
	$ticker1 = flame\time\tick(100, function($ticker1) use(&$count) {
		++$count;
		if($count > 10) {
			$ticker1->stop();
			return;
		}
		echo "[count]= ", $count, "\n";
	});
});
// 协程调度开始
flame\run();
