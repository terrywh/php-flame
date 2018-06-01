<?php
// 进程名称将被设置为（不含引号）： "basic_test (flame-master)"
flame\init("basic_test");

flame\go(function() {
	// yield flame\time\sleep(10);
	echo "done\n";
});

// 运行协程调度（阻塞在此处，实际程序在内部调度循环）
flame\run();
