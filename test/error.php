<?php
dl("flame.so");
// 创建子进程 1 个（目前没有提供区分、识别父子进程的方式，理论上父子进程运行代码相同）
// flame\fork(1);
class class_test {
	public static function aaaa() {
		echo "Aaaaa\n";
	}
	public function bbbb() {
		aaaa();
	}
}
flame\run(function() {
	$ct = new class_test();
	flame\go(array($ct, "bbbb"));
	yield flame\sleep(10000);
});
