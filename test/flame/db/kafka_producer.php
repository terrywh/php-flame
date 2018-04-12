<?php
flame\init("kafka_producer");
flame\go(function() {
	$producer = new flame\db\kafka\producer([
		"bootstrap.servers" => "11.22.33.44:9092",
	], [], "wuhao-test");
	
	$exit = false;
	$tick = flame\time\after(600000, function() use(&$exit) {
		// 600s 后退出
		$exit = true;
	});
SEND_ANOTHER_1000:
	$begin = microtime(true);
	$i = 0;
	while(!$exit) {
		yield $producer->produce("".$i);
		if(++$i % 10000 == 0) {
			yield $producer->flush();
			echo "flush\n";
		}
	}
	$end = microtime(true);
	echo "total ", $i, " msgs, produced in ", $end - $begin, "s\n";
	// yield flame\time\sleep(5000);
	// goto SEND_ANOTHER_10000;
	$tick->stop();
});
flame\run();
