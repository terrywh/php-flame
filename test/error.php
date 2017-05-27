<?php
dl("flame.so");

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
