<?php
flame\init("kafka_1");
flame\go(function() {
	// ob_start();
	$consumer = yield flame\kafka\consume([
        "bootstrap.servers" => "47.93.28.184:9092",
        "group.id" => "flame-test-consumer",
		"auto.offset.reset" => "smallest",
    ], ["commas-wuhao-0x0001"]);
	$count = 0;
	flame\time\after(50000, function() use($consumer) {
		echo "before close\n";
		$consumer->close();
		echo "after close\n";
	});
	echo 1,"\n";
	yield $consumer->run(function($msg) use(&$count, $consumer) {
		echo ++$count, ": ";
		var_dump($msg);
		// 可选，自行提交
	 	// $consumer->commit($msg);
	});
	echo 2,"\n";
	yield flame\time\sleep(1000);
});

flame\run();
