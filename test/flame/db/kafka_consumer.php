<?php
flame\init("kafka_consumer");
flame\go(function() {
	$consumer = new flame\db\kafka\consumer([
		"bootstrap.servers" => "10.20.6.59:9092",
		"group.id" => "console-consumer-58548",
		"fetch.message.max.bytes" => 64 * 1024,
	], [
		"auto.offset.reset" => "largest", // lastest
	], ["wuhao-test"]);
	// $count = 0;
	while(true) {
		$msg = yield $consumer->consume();
		echo $msg->time, " -> ", $msg, "\n";
	}
});
flame\run();
