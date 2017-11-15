<?php
flame\init("kafka_test");
flame\go(function() {
	$consumer = new flame\db\kafka\consumer([
		"bootstrap.servers" => "10.20.6.59:9092",
		"group.id" => "console-consumer-58548",
	], [
		"auto.offset.reset" => "earliest",
	], ["wuhao-test"]);
	yield $consumer->consume(function($msg) use(&$consumer) {
		echo $msg->time, " -> ", $msg, "\n";
		yield $consumer->commit($msg);
	});
});
flame\run();
