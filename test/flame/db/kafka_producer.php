<?php
flame\init("kafka_producer");
flame\go(function() {
	$producer = new flame\db\kafka\producer([
		"bootstrap.servers" => "10.20.6.59:9092",
	], [], "wuhao-test");
	$begin = microtime(true);
	$exit = false;
	flame\time\after(600000, function() use(&$exit) {
		$exit = true;
	});
	while(!$exit) {
		yield $producer->produce("".$i);
	}
	$end = microtime(true);
	echo "produced in ", $end - $begin, "s\n";
});
flame\run();
