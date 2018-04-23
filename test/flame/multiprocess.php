<?php
// 进程名称将被设置为
// 主进程："worker test (fm)"
// 子进程："worker test (fw)"
flame\init("worker test", [
	"worker" => 2, // 总共 3 个进程（工作进程 2 个 主进程 1 个）
]);
flame\go(function() {
	for($i=0;$i<10;++$i) {
		yield flame\time\sleep(1000);
		echo "(", getenv("FLAME_CLUSTER_WORKER"), ") [", posix_getpid(), "] -> ", $i, "\n";
	}
});
flame\run();
