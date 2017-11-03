<?php
flame\init("log_test", [
	"worker"=> 8,
]);
flame\go(function() {
	flame\log\set_output("/tmp/test.log");
	for($i=0;$i<100;++$i) {
		yield flame\time\sleep(rand(0,50));
		yield flame\log\fail(rand(), str_pad("".$i, 20, "0"), ["a"=>"b"]);
	}
});
flame\run();
