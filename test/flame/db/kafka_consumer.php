<?php
flame\init("kafka_consumer");
flame\go(function() {
	$consumer = new flame\db\kafka\consumer([
		"bootstrap.servers" => "10.20.6.59:9092",
		"group.id" => "console-consumer-58548",
		// "fetch.message.max.bytes" => 64 * 1024,
	], [
		"auto.offset.reset" => "largest", // lastest
	], ["wuhao-test"]);
	$count = 0;
	$exit  = false;
	$tick = flame\time\after(600000, function() use(&$exit) {
		// 600s 后退出
		$exit = true;
	});
	$begin = microtime(true);
	while(!$exit) {
		$msg = yield $consumer->consume();
		++ $count;
		echo $msg->timestamp_ms(), " => ", $msg, "\n";
	}
	$end = microtime(true);
	echo $count, " messages consumed in ", $end - $begin, "s\n";
	$tick->stop();
});
flame\run();
