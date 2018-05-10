<?php
flame\init("rabbit_producer");
flame\go(function() {
	$producer = new flame\db\rabbitmq\producer(
		"amqp://wuhao:123456@11.22.33.44:5672/wuhao",
		[], // options
		"" // exchange
	);
	
	$exit = false;
	$tick = flame\time\after(600000, function() use(&$exit) {
		// 600s 后退出
		$exit = true;
	});
SEND_ANOTHER_10000:
	$begin = microtime(true);
	$i = 0;
	while(!$exit) {
		yield $producer->produce("".(++$i), "flame-test", ["headers"=>["aaa"=>"bbb"],"app_id"=>"ccc"]);
		if(++$i % 10000 == 0) {
			yield $producer->flush();
			echo "flush\n";
		}
		yield flame\time\sleep(1);
	}
	$end = microtime(true);
	echo "total ", $i, " msgs, produced in ", $end - $begin, "s\n";
	// yield flame\time\sleep(5000);
	// goto SEND_ANOTHER_10000;
	$tick->stop();
});
flame\run();
