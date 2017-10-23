<?php
// 进程名称将被设置为：（不含引号）
// "coroutine test (flame-master)"
flame\init("coroutine_test");
// 第一个协程
flame\go(function() {
	for($i=0;$i<10;++$i) {
		// 使用 flame 提供的 sleep（异步调度而非阻塞）
		yield flame\time\sleep(500);
		echo 1, " [", microtime(true),"]\n";
	}
});
// 第二个协程
flame\go(function() {
	for($i=0;$i<10;++$i) {
		// 两个协程可以“独立”、“互不影响”的工作
		yield flame\time\sleep(800);
		echo 2, " [", microtime(true),"]\n";
	}
});
// 运行协程调度（阻塞在此处，实际程序在内部调度循环）
flame\run();
