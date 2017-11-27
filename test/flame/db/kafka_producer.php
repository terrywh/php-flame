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
	$i = 0;
	while(!$exit) {
		yield $producer->produce("".$i);
		++$i;
		if($i % 10000 == 0) yield $producer->flush();
	}
	$end = microtime(true);
	echo "produced in ", $end - $begin, "s\n";
});
flame\run();
