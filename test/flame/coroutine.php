<?php
// 进程名称将被设置为：（不含引号）
// "coroutine test (flame-master)"
// flame\init("coroutine_test", ["worker"=>4]);
flame\init("coroutine_test");

function test_1($i) {
	echo $i. "\n";
	yield test_2($i); // yield from test_2();
	yield flame\time\sleep(10);
}
function test_2($i) {
	yield flame\time\sleep(10);
	flame\time\after(1000, function() use($i) {
		echo "\t$i\txyz\n";
	});
}

flame\go(function() {
	for($i=0;$i<10;++$i) {
		yield flame\time\sleep(50);
		flame\go(function() use($i) {
			
			yield test_1($i);
		});
	}
	yield flame\time\sleep(1000);
	echo "done\n";
});

// 运行协程调度（阻塞在此处，实际程序在内部调度循环）
flame\run();
