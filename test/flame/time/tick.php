<?php
flame\init("time_tick_test");
flame\go(function() {
	$count = 0;
	$tick = new flame\time\ticker(1000/*, true */);
	$tick->start(function($tick) use(&$count) {
		++$count;
		if($count > 4) {
			$tick->stop();
		}
		echo "count= ", $count, "\n";
	});
});
flame\go(function() {
	$total = 0;
	// 简化调用，与上述 ticker 定时器功能过程功能一致
	$tick = flame\time\tick(1000, function() use(&$total) {
		++$total;
		echo "total= ", $total, "\n";
	});
	// 6s 后执行回调
	flame\time\after(6000, function() use(&$total, &$tick) {
		echo "stop= ", $total, "\n";
		$tick->stop();
	});
	// 上面 after 函数过程，相当于：
	// $tick = new flame\time\ticker(6000, false);
	// $tick->start(function() {});
	// 即：非重复计时器
});
// 协程调度开始
flame\run();
