<?php
flame\init("kafka_producer");
flame\go(function() {
	$producer = new flame\db\rabbitmq\producer("amqp://wuhao:RpNSqYcT5EQNo7V3@10.20.6.71:5672/wuhao?heartbeat=15");
	
	$exit = false;
	$tick = flame\time\after(600000, function() use(&$exit) {
		// 600s 后退出
		$exit = true;
	});
SEND_ANOTHER_10000:
	$begin = microtime(true);
	$i = 0;
	while(!$exit) {
		yield $producer->produce("".$i, "flame-test", ["headers"=>["aaa"=>"bbb"],"app_id"=>"ccc"]);
		break;
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
