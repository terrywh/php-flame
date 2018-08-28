<?php
flame\init("rabbitmq_1");
flame\go(function() {
	ob_start();
	$count = 0;
	$client = yield flame\rabbitmq\connect("amqp://wuhao:123456@11.22.33.44:5672/vhost");

	$producer = $client->produce();
	for($i=0;$i<100;++$i) {
		$producer->publish(rand(), "flame-test");
		++$count;
		yield flame\time\sleep(10);
		$message = new flame\rabbitmq\message(rand(), "flame-test");
		$message->header["a"] = "bb";
		// $message->header["b"] = flame\time\now();
		$message->header["c"] = 123.123;
		$message->content_type = "text/plain";
		$message->timestamp = intval(flame\time\now()/1000);
		$producer->publish($message);
		++$count;
		yield flame\time\sleep(10);
	}
	yield flame\time\sleep(1000); // 稍微给点时间确认消费完

	echo "done:{$count}.\n";
	$output = ob_get_flush();
	assert($output == "done:200.\n");
});
flame\run();
