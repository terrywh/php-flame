<?php
flame\init("time_ticker");
flame\go(function() {
	$count = 0;
	$ticker2 = new flame\time\ticker(100/*, true */);
	$ticker2->start(function($ticker2) use(&$count) {
		++$count;
		if($count > 10) {
			$ticker2->stop();
			return;
		}
		echo "(count)= ", $count, "\n";
	});
});
// 协程调度开始
flame\run();
