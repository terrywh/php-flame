<?php
ob_start();

flame\init("redis_1");
flame\go(function() {
	$cli = yield flame\redis\connect("redis://auth:123456@11.22.33.44:6379/0");
	
	yield $cli->set("test", "first value");
	$m = yield $cli->multi();
	assert($m instanceof flame\redis\transaction);
	$r = yield $m // TRANSACTION
		->get("test")
		->set("test", "second value")
		->get("test")
		->exec();
	assert($r[0] == "first value");
	assert($r[1] == "OK");
	assert($r[2] == "second value");
	
	$r = yield $cli->pipel() // PIPELINE
	 	->get("test")
		->set("test", "third value")
		->get("test")
	 	->exec();
	assert($r[0] == "second value");
	assert($r[1] == "OK");
	assert($r[2] == "third value");
	echo "done1.\n";
});
flame\run();

if(getenv("FLAME_PROCESS_WORKER")) {
	$output = ob_get_flush();
	assert($output == "done1.\n");
}