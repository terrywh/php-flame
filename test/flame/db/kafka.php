<?php
flame\init("kafka_test");
flame\go(function() {
	$producer = new flame\db\kafka\producer([
		"bootstrap.servers" => "10.20.6.59:9092",
	], [], "wuhao-test");
	$begin = microtime(true);
	for($i=0;$i<100;++$i) {
		yield $producer->produce("".rand());
	}
	$end = microtime(true);
	echo "finished in ", $end - $begin, "s\n";
});
flame\run();
