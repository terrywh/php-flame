<?php
flame\init("kafka_test");
flame\go(function() {
	$producer = new flame\db\kafka\producer([
		"bootstrap.servers" => "10.20.6.59:9092",
	], [], "wuhao-test");
	$begin = microtime(true);
	for($i=0;$i<10;++$i) {
		yield $producer->produce("".$i);
	}
	$end = microtime(true);
	echo "produced in ", $end - $begin, "s\n";
});
flame\run();
