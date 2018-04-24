<?php
flame\init("log_test", [
	"worker"=> 0,
]);
flame\go(function() {
	flame\log\set_output("/tmp/test.log");
	for($i=0;$i<100;++$i) {
		yield flame\time\sleep(1000);
		echo $i,"\n";
		yield flame\log\fail(rand(), str_pad("".$i, 20, "0"), ["a"=>"b"]);
	}
});
flame\run();
