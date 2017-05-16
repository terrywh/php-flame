<?php
dl("flame.so");
// 创建子进程 1 个（目前没有提供区分、识别父子进程的方式，理论上父子进程运行代码相同）
// flame\fork(1);
flame\run(function() {
	// 启动 “协程”
	// “协程” 将在 run 之后被分别调度执行
	flame\go(test1());
	// “协程” 允许使用以下两种形式：
	// 1. 带有 yield 形式的函数定义（调用后会返回 Generator 对象）;
	// 2. Generator 对象；
	// flame\go(test2);
	flame\go(test3());
});

function test1() {
	for($i=0;$i<1;++$i) {
		// 需要使用 yield 形式调用 flame 提供的 sleep 函数，以此来实现 “调度式阻塞” 并行逻辑
		yield flame\sleep(100);
		echo "[1] " . time() . " -> " . $i . "\n";
	}
}
function test2() {
	for($i=0;$i<10;++$i) {
		yield flame\sleep(200);
		echo "[2] " . time() . " -> " . $i . "\n";
	}
}

function test3() {
	for($i=0;$i<10;++$i) {
		echo "[3] $i\n";
		// 使用 async 函数，将同步任务转化为异步（借助一个工作线程）
		// 请一定谨慎使用，错误的使用会因多线程并发而引起各种诡异的错乱问题
		// 注意，额外的工作线程仅有 一个，故不能用于解决多核问题
		// （CPU 消耗过高，或过多的阻塞任务一样会占用这个工作线程）
		echo yield flame\async(function($done) use($i) {
			usleep(300000);
			$done(null, "[3] " . time() . " -> ". $i . "\n");
		});
	}
}
