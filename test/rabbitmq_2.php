<?php
ob_start();

flame\init("rabbitmq_1");
flame\go(function() {
	$count = 0;
	$client = yield flame\rabbitmq\connect("amqp://wuhao:123456@11.22.33.44:5672/vhost");
	
	$producer = $client->produce();
	for($i=0;$i<100;++$i) {
		$producer->publish(rand(), "flame-test");
		++$count;
		yield flame\time\sleep(5);
		$message = new flame\rabbitmq\message(rand());
		$message->header["a"] = "bb";
		$message->content_type = "text/plain";
		$producer->publish($message, "flame-test");
		++$count;
		yield flame\time\sleep(5);
	}
	yield flame\time\sleep(1000); // 稍微给点时间确认消费完

	echo "done:{$count}.\n";
});
flame\run();

if(getenv("FLAME_PROCESS_WORKER")) {
	$output = ob_get_flush();
	assert($output == "done:200.\n");
}