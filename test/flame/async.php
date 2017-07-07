<?php
flame\go(function() {
	// async 函数用于将 “同步”编程“异步”，
	// 传递给 async 的函数将在 工作线程池 中运行，
	// 在使用共享资源时请小心注意；
	$g = 0;
	for($i=0;$i<10000000;++$i) {
		yield flame\async(function() use(&$g) {
			++$g;
			sleep(1);
		});
		echo $g, "\n";
	}
	
});
flame\go(function() {
	// async 函数用于将 “同步”编程“异步”，
	// 传递给 async 的函数将在 工作线程池 中运行，
	// 在使用共享资源时请小心注意；
	// $rv = yield flame\async(function() {
	// 	// sleep(3);
	// 	return 3333;
	// });
	for($i=0;$i<10000000;++$i) {
		yield flame\time\sleep(1000);
	}
});


flame\run();
