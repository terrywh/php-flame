<?php
dl("flame.so");
flame\fork(1);
// 启动 “协程”
// “协程” 将在 run 之后被分别调度执行
flame\go(test1());
// “协程” 允许使用以下两种形式：
// 1. 带有 yield 形式的函数定义（调用后会返回 Generator 对象）;
// 2. Generator 对象；
flame\go(test2);
// 下面两行 go + run 可以合并为一行：
// flame\run(test3());
flame\go(test3());
flame\run();
function test1() {
	for($i=0;$i<10;++$i) {
		// 需要使用 yield 形式调用 flame 提供的 sleep 函数，以此来实现 “调度式阻塞” 并行逻辑
		yield flame\sleep(500);
		echo "[1] ", time(), " -> ", $i,"\n";
	}
}
function test2() {
	for($i=0;$i<10;++$i) {
		yield flame\sleep(700);
		echo "[2] ", time(), " -> ", $i,"\n";
	}
}

function test3() {
	for($i=0;$i<10;++$i) {
		// 使用 async 函数，将同步任务转化为异步（借助线程池）
		// 请一定谨慎使用，错误的使用会因多线程并发而引起各种诡异的错乱问题
		yield flame\async(function($done) {
			sleep(10); // 系统 sleep 是阻塞函数
			done();
		});
		echo "[3] ", time(), " -> ", $i,"\n";
	}
}
function test4() {
	echo yield flame\async(function($done) {
		done(null, "aaaaaaaaaa")
	}), "\n";
}
