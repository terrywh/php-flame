<?php
flame\init("rabbitmq_1");
flame\go(function() {
	ob_start();
	// $client = yield flame\rabbitmq\connect("amqp://wuhao:123456@11.22.33.44:5672/vhost");
	$count = 0;
	$consumer = $client->consume("xypk:gift-10.20.6.51");
	flame\time\after(20000, function() use($consumer) {
		yield $consumer->close();
	});
	yield $consumer->run(function($msg) use(&$count, $consumer) {
		++$count;
		assert($msg != null);
	 	// $consumer->confirm($msg);
	});

	echo "done:{$count}.\n";
	$output = ob_get_flush();
	assert($output == "done:200.\n");
});
flame\run();
