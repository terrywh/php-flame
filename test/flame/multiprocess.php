<?php
flame\init("multiprocess", [
	"worker" => 1,
]);
flame\go(function() {
	yield flame\time\sleep(60000);
});
flame\on_quit(function() {
	echo "ending.\n";
	// 由于这里没有停止上述 sleep，故：
	// 1. 当上述 sleep 超过停止超时 10s 则会被提前终止；
	// 2. 当上述 sleep 在 10s 内结束，进程立刻终止；
});
flame\run();
