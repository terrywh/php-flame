<?php
flame\init("time_after");
flame\go(function() {
	for($i=0;$i<100;++$i) {
		yield flame\time\sleep(50);
		/*$after = */flame\time\after(1000, function($after) {
			yield 1;
			echo flame\time\now(), "\n";
		});
		// 可以使用：
		// $after->stop()
		// 提前终止
	}
});
// 协程调度开始
flame\run();
