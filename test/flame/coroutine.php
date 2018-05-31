<?php
// 进程名称将被设置为：（不含引号）
// "coroutine test (flame-master)"
flame\init("coroutine_test", ["worker"=>4]);

function test_coroutine_1() {
	for($i=0;$i<10;++$i) {
		echo "\t". $i. "\n";
		yield from test_coroutine_2();
		yield flame\time\sleep(10);
	}
}
function test_coroutine_2() {
	yield flame\time\sleep(10);
	flame\time\after(1000, function() {
		echo "\txyz\n";
	});
	yield test_coroutine_3();
}
function test_coroutine_3() {
	yield flame\time\sleep(10);
	flame\time\after(1000, function() {
		echo "\tabc\n";
	});
	yield flame\time\sleep(10);
}
flame\go(function() {
	for($i=0;$i<10;++$i) {
		yield flame\time\sleep(50);
		flame\go(function() use($i) {
			echo $i. "\n";
			yield test_coroutine_1();
		});
	}
	yield flame\time\sleep(1000);
	echo "done\n";
});

// 运行协程调度（阻塞在此处，实际程序在内部调度循环）
flame\run();
