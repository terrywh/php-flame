<?php
ob_start();

flame\init("core_1");
flame\go(function() {
	// 1. 程名称设置
	$n = getenv("FLAME_PROCESS_WORKER");
	assert("core_1 (flame/".$n.")" == cli_get_process_title());
	// 2. 协程暂停和恢复
	assert(1 == yield 1);
	assert(2 == yield 2);
	assert(3 == yield 3);
	echo "done1.\n";
});
flame\go(function() {
	assert(true);
	echo "done2.\n";
});
flame\run();

if(getenv("FLAME_PROCESS_WORKER")) {
	$output = ob_get_flush();
	assert($output == "done2.\ndone1.\n");
}