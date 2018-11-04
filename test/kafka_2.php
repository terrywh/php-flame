<?php
flame\init("kafka_2");
flame\go(function() {
	// ob_start();
	$producer = yield flame\kafka\produce([
        "bootstrap.servers" => "47.93.28.184:9092",
    ], ["commas-wuhao-0x0001"]);
	for($i=0; $i<100; ++$i) {
		yield flame\time\sleep(10);
        yield $producer->publish("commas-wuhao-0x0001", "abc", "xyz");
		$msg = new flame\kafka\message("abc", "xyz");
		$msg->header["A"]="B";
		yield $producer->publish("commas-wuhao-0x0001", $msg);
    }
	yield $producer->flush();
	echo "done.\n";
});

flame\run();
